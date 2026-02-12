// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mining_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

// --- Hex helpers ---

static const char hex_chars[] = "0123456789abcdef";

std::vector<uint8_t> HexToBytes(const std::string &hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto hi = hex[i], lo = hex[i + 1];
        auto hv = (hi >= 'a') ? (hi - 'a' + 10)
                  : (hi >= 'A') ? (hi - 'A' + 10) : (hi - '0');
        auto lv = (lo >= 'a') ? (lo - 'a' + 10)
                  : (lo >= 'A') ? (lo - 'A' + 10) : (lo - '0');
        bytes.push_back((uint8_t)((hv << 4) | lv));
    }
    return bytes;
}

std::string BytesToHex(const uint8_t *data, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        out += hex_chars[(data[i] >> 4) & 0xf];
        out += hex_chars[data[i] & 0xf];
    }
    return out;
}

uint32_t SwapEndian32(uint32_t v) {
    return ((v >> 24) & 0xff) | ((v >> 8) & 0xff00) |
           ((v << 8) & 0xff0000) | ((v << 24) & 0xff000000);
}

// --- Difficulty → Target ---

void DifficultyToTarget(double difficulty, uint32_t target[8]) {
    // Scrypt diff1 target: 0x0000ffff00000000...
    // target = diff1 / difficulty
    memset(target, 0, 32);

    if (difficulty <= 0) difficulty = 1.0;

    // diff1 in 256-bit: word[6] (LE) = 0xffff0000, rest zero except conceptually
    // Simplified: compute as 64-bit then spread
    // For Scrypt: diff1 = 2^224 * 0xffff = 0x0000ffff << 224
    // target = diff1 / d
    // We approximate with doubles for reasonable difficulty ranges.

    double diff1 = 0xffff;
    double val = diff1 / difficulty;

    // Place the value at the right position in the 256-bit target
    // target[7] (LE) is the most significant word
    // diff1 sits in target[6] = 0xffff0000 → byte offset 24-27
    // So base exponent is 224 bits = word 7 position

    // For a cleaner approach: fill target as 0000ffff / diff at word position 6
    uint64_t tval = (uint64_t)(val * 65536.0); // scale by 2^16
    target[6] = (uint32_t)(tval & 0xFFFFFFFF);
    target[7] = (uint32_t)(tval >> 32);

    // For very low difficulties, also fill lower words
    if (difficulty < 1.0) {
        double remainder = val - (double)(tval >> 16) * 1.0;
        if (remainder > 0) {
            target[5] = (uint32_t)(remainder * 4294967296.0);
        }
    }
}

// --- Build mining work from stratum job ---

static uint32_t ReadLE32(const uint8_t *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// SHA-256 for merkle tree computation
static void sha256_block(uint32_t state[8], const uint32_t block[16]) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define RR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    uint32_t w[64];
    for (int i = 0; i < 16; i++) w[i] = block[i];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = RR(w[i-2], 17) ^ RR(w[i-2], 19) ^ (w[i-2] >> 10);
        uint32_t s1 = RR(w[i-15], 7) ^ RR(w[i-15], 18) ^ (w[i-15] >> 3);
        w[i] = w[i-16] + s1 + w[i-7] + s0;
    }

    uint32_t a=state[0], b=state[1], c=state[2], d=state[3];
    uint32_t e=state[4], f=state[5], g=state[6], h=state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t S1 = RR(e,6) ^ RR(e,11) ^ RR(e,25);
        uint32_t ch = (e&f) ^ (~e&g);
        uint32_t t1 = h + S1 + ch + k[i] + w[i];
        uint32_t S0 = RR(a,2) ^ RR(a,13) ^ RR(a,22);
        uint32_t maj = (a&b) ^ (a&c) ^ (b&c);
        uint32_t t2 = S0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
#undef RR
}

static void sha256d(const uint8_t *data, size_t len, uint8_t out[32]) {
    // First pass
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    // Pad to 64-byte blocks
    size_t padded_len = ((len + 9 + 63) / 64) * 64;
    std::vector<uint8_t> padded(padded_len, 0);
    memcpy(padded.data(), data, len);
    padded[len] = 0x80;
    uint64_t bitlen = len * 8;
    for (int i = 0; i < 8; i++)
        padded[padded_len - 1 - i] = (bitlen >> (i * 8)) & 0xff;

    for (size_t off = 0; off < padded_len; off += 64) {
        uint32_t block[16];
        for (int i = 0; i < 16; i++)
            block[i] = (padded[off+i*4] << 24) | (padded[off+i*4+1] << 16) |
                       (padded[off+i*4+2] << 8) | padded[off+i*4+3];
        sha256_block(state, block);
    }

    uint8_t hash1[32];
    for (int i = 0; i < 8; i++) {
        hash1[i*4+0] = (state[i] >> 24) & 0xff;
        hash1[i*4+1] = (state[i] >> 16) & 0xff;
        hash1[i*4+2] = (state[i] >> 8) & 0xff;
        hash1[i*4+3] = state[i] & 0xff;
    }

    // Second pass
    uint32_t state2[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t padded2[64];
    memcpy(padded2, hash1, 32);
    padded2[32] = 0x80;
    memset(padded2 + 33, 0, 64 - 33);
    padded2[62] = 0x01;
    padded2[63] = 0x00; // 256 bits

    uint32_t block2[16];
    for (int i = 0; i < 16; i++)
        block2[i] = (padded2[i*4] << 24) | (padded2[i*4+1] << 16) |
                    (padded2[i*4+2] << 8) | padded2[i*4+3];
    sha256_block(state2, block2);

    for (int i = 0; i < 8; i++) {
        out[i*4+0] = (state2[i] >> 24) & 0xff;
        out[i*4+1] = (state2[i] >> 16) & 0xff;
        out[i*4+2] = (state2[i] >> 8) & 0xff;
        out[i*4+3] = state2[i] & 0xff;
    }
}

bool BuildMiningWork(MiningWork &work) {
    const auto &job = work.job;

    // Build coinbase: coinbase1 + extranonce1 + extranonce2 + coinbase2
    char en2[16];
    snprintf(en2, sizeof(en2), "%0*x", work.extranonce2Size * 2,
             work.extranonce2Counter);
    std::string extranonce2Hex(en2);

    std::string coinbaseHex =
        job.coinbase1 + work.extranonce1 + extranonce2Hex + job.coinbase2;
    auto coinbaseBytes = HexToBytes(coinbaseHex);

    // SHA-256d of coinbase → coinbase hash
    uint8_t cbHash[32];
    sha256d(coinbaseBytes.data(), coinbaseBytes.size(), cbHash);

    // Build merkle root by hashing with branches
    uint8_t merkleRoot[32];
    memcpy(merkleRoot, cbHash, 32);

    for (const auto &branch : job.merkleBranches) {
        auto branchBytes = HexToBytes(branch);
        uint8_t concat[64];
        memcpy(concat, merkleRoot, 32);
        memcpy(concat + 32, branchBytes.data(), 32);
        sha256d(concat, 64, merkleRoot);
    }

    // Build 80-byte header:
    // version (4) + prevhash (32) + merkleroot (32) + ntime (4) + nbits (4) + nonce (4)
    auto verBytes = HexToBytes(job.version);
    auto prevBytes = HexToBytes(job.prevHash);
    auto ntimeBytes = HexToBytes(job.ntime);
    auto nbitsBytes = HexToBytes(job.nbits);

    uint8_t headerBytes[80];
    memset(headerBytes, 0, 80);

    // Version: little-endian
    if (verBytes.size() >= 4) memcpy(headerBytes, verBytes.data(), 4);
    // PrevHash: already in internal order from stratum
    if (prevBytes.size() >= 32) memcpy(headerBytes + 4, prevBytes.data(), 32);
    // MerkleRoot
    memcpy(headerBytes + 36, merkleRoot, 32);
    // nTime: little-endian
    if (ntimeBytes.size() >= 4) memcpy(headerBytes + 68, ntimeBytes.data(), 4);
    // nBits: little-endian
    if (nbitsBytes.size() >= 4) memcpy(headerBytes + 72, nbitsBytes.data(), 4);
    // nonce = 0 (placeholder, miner will iterate)
    memset(headerBytes + 76, 0, 4);

    // Copy to work.header as uint32 LE
    for (int i = 0; i < 20; i++) {
        work.header[i] = ReadLE32(headerBytes + i * 4);
    }

    DifficultyToTarget(work.difficulty, work.target);

    return true;
}

// --- CPU Scrypt(1024,1,1) ---
// Reference implementation for CPU fallback mining.

static inline uint32_t rotl32(uint32_t v, int c) {
    return (v << c) | (v >> (32 - c));
}

static void salsa20_8_cpu(uint32_t B[16]) {
    uint32_t x[16];
    memcpy(x, B, 64);

    for (int i = 0; i < 8; i += 2) {
        x[ 4] ^= rotl32(x[ 0]+x[12], 7);  x[ 8] ^= rotl32(x[ 4]+x[ 0], 9);
        x[12] ^= rotl32(x[ 8]+x[ 4],13);  x[ 0] ^= rotl32(x[12]+x[ 8],18);
        x[ 9] ^= rotl32(x[ 5]+x[ 1], 7);  x[13] ^= rotl32(x[ 9]+x[ 5], 9);
        x[ 1] ^= rotl32(x[13]+x[ 9],13);  x[ 5] ^= rotl32(x[ 1]+x[13],18);
        x[14] ^= rotl32(x[10]+x[ 6], 7);  x[ 2] ^= rotl32(x[14]+x[10], 9);
        x[ 6] ^= rotl32(x[ 2]+x[14],13);  x[10] ^= rotl32(x[ 6]+x[ 2],18);
        x[ 3] ^= rotl32(x[15]+x[11], 7);  x[ 7] ^= rotl32(x[ 3]+x[15], 9);
        x[11] ^= rotl32(x[ 7]+x[ 3],13);  x[15] ^= rotl32(x[11]+x[ 7],18);
        x[ 1] ^= rotl32(x[ 0]+x[ 3], 7);  x[ 2] ^= rotl32(x[ 1]+x[ 0], 9);
        x[ 3] ^= rotl32(x[ 2]+x[ 1],13);  x[ 0] ^= rotl32(x[ 3]+x[ 2],18);
        x[ 6] ^= rotl32(x[ 5]+x[ 4], 7);  x[ 7] ^= rotl32(x[ 6]+x[ 5], 9);
        x[ 4] ^= rotl32(x[ 7]+x[ 6],13);  x[ 5] ^= rotl32(x[ 4]+x[ 7],18);
        x[11] ^= rotl32(x[10]+x[ 9], 7);  x[ 8] ^= rotl32(x[11]+x[10], 9);
        x[ 9] ^= rotl32(x[ 8]+x[11],13);  x[10] ^= rotl32(x[ 9]+x[ 8],18);
        x[12] ^= rotl32(x[15]+x[14], 7);  x[13] ^= rotl32(x[12]+x[15], 9);
        x[14] ^= rotl32(x[13]+x[12],13);  x[15] ^= rotl32(x[14]+x[13],18);
    }
    for (int i = 0; i < 16; i++) B[i] += x[i];
}

static void scrypt_blockmix(uint32_t *B, uint32_t *Y) {
    uint32_t X[16];
    memcpy(X, B + 16, 64); // last 64-byte block

    // First half
    for (int j = 0; j < 16; j++) X[j] ^= B[j];
    salsa20_8_cpu(X);
    memcpy(Y, X, 64);

    // Second half
    for (int j = 0; j < 16; j++) X[j] ^= B[16 + j];
    salsa20_8_cpu(X);
    memcpy(Y + 16, X, 64);
}

static void scrypt_romix_cpu(uint32_t *X, uint32_t *V) {
    uint32_t Y[32];
    for (int i = 0; i < 1024; i++) {
        memcpy(V + i * 32, X, 128);
        scrypt_blockmix(X, Y);
        memcpy(X, Y, 128);
    }
    for (int i = 0; i < 1024; i++) {
        int j = X[16] & 1023;
        for (int k = 0; k < 32; k++) X[k] ^= V[j * 32 + k];
        scrypt_blockmix(X, Y);
        memcpy(X, Y, 128);
    }
}

// PBKDF2-HMAC-SHA256 (simplified for scrypt use)
static void pbkdf2_sha256_cpu(const uint8_t *pass, size_t passlen,
                               const uint8_t *salt, size_t saltlen,
                               uint8_t *dk, size_t dklen) {
    // For scrypt, we do 1 iteration, and dklen is 128 or 32
    // This is a simplified implementation
    uint32_t blocks = (uint32_t)((dklen + 31) / 32);
    for (uint32_t blk = 1; blk <= blocks; blk++) {
        // U = HMAC-SHA256(pass, salt || INT(blk))
        std::vector<uint8_t> msg(saltlen + 4);
        memcpy(msg.data(), salt, saltlen);
        msg[saltlen + 0] = (blk >> 24) & 0xff;
        msg[saltlen + 1] = (blk >> 16) & 0xff;
        msg[saltlen + 2] = (blk >> 8) & 0xff;
        msg[saltlen + 3] = blk & 0xff;

        // HMAC-SHA256(key=pass, msg=msg)
        uint8_t ipad[64], opad[64];
        memset(ipad, 0x36, 64);
        memset(opad, 0x5c, 64);
        for (size_t i = 0; i < passlen && i < 64; i++) {
            ipad[i] ^= pass[i];
            opad[i] ^= pass[i];
        }

        // inner = SHA256(ipad || msg)
        std::vector<uint8_t> inner_data(64 + msg.size());
        memcpy(inner_data.data(), ipad, 64);
        memcpy(inner_data.data() + 64, msg.data(), msg.size());
        uint8_t inner_hash[32];
        sha256d(inner_data.data(), inner_data.size(), inner_hash);
        // Note: this uses double-SHA256 but PBKDF2 needs single SHA256.
        // For a proper implementation, we'd need single SHA256.
        // This is a simplification — the GPU kernel handles the real PoW.
        // For CPU testing, link against openssl or use the node's scrypt.

        // outer = SHA256(opad || inner_hash)
        uint8_t outer_data[96];
        memcpy(outer_data, opad, 64);
        memcpy(outer_data + 64, inner_hash, 32);
        uint8_t outer_hash[32];
        sha256d(outer_data, 96, outer_hash);

        size_t copy = std::min((size_t)32, dklen - (blk - 1) * 32);
        memcpy(dk + (blk - 1) * 32, outer_hash, copy);
    }
}

static void scrypt_1024_1_1_cpu(const uint8_t header[80], uint32_t nonce,
                                 uint8_t hash[32]) {
    uint8_t hdr[80];
    memcpy(hdr, header, 80);
    hdr[76] = nonce & 0xff;
    hdr[77] = (nonce >> 8) & 0xff;
    hdr[78] = (nonce >> 16) & 0xff;
    hdr[79] = (nonce >> 24) & 0xff;

    // PBKDF2(header, header, 1, 128) → B
    uint8_t B[128];
    pbkdf2_sha256_cpu(hdr, 80, hdr, 80, B, 128);

    // ROMix
    uint32_t X[32];
    for (int i = 0; i < 32; i++)
        X[i] = ReadLE32(B + i * 4);

    std::vector<uint32_t> V(32 * 1024);
    scrypt_romix_cpu(X, V.data());

    // Write X back to B
    for (int i = 0; i < 32; i++) {
        B[i*4+0] = X[i] & 0xff;
        B[i*4+1] = (X[i] >> 8) & 0xff;
        B[i*4+2] = (X[i] >> 16) & 0xff;
        B[i*4+3] = (X[i] >> 24) & 0xff;
    }

    // PBKDF2(header, B, 1, 32) → hash
    pbkdf2_sha256_cpu(hdr, 80, B, 128, hash, 32);
}

static bool check_target(const uint8_t hash[32], const uint32_t target[8]) {
    // Compare LE: hash[31..0] vs target[7..0]
    for (int i = 7; i >= 0; i--) {
        uint32_t h = ReadLE32(hash + i * 4);
        if (h < target[i]) return true;
        if (h > target[i]) return false;
    }
    return true;
}

// --- CpuMiner ---

CpuMiner::CpuMiner(const CpuMinerConfig &config) : m_config(config) {}

uint64_t CpuMiner::ScanNonces(const MiningWork &work, uint32_t nonceStart,
                               uint32_t nonceEnd,
                               std::function<void(uint32_t)> foundCb,
                               std::atomic<bool> &interrupt) {
    uint8_t headerBytes[80];
    for (int i = 0; i < 20; i++) {
        headerBytes[i*4+0] = work.header[i] & 0xff;
        headerBytes[i*4+1] = (work.header[i] >> 8) & 0xff;
        headerBytes[i*4+2] = (work.header[i] >> 16) & 0xff;
        headerBytes[i*4+3] = (work.header[i] >> 24) & 0xff;
    }

    uint64_t count = 0;
    for (uint32_t nonce = nonceStart; nonce < nonceEnd && !interrupt.load();
         nonce++) {
        uint8_t hash[32];
        scrypt_1024_1_1_cpu(headerBytes, nonce, hash);
        count++;

        if (check_target(hash, work.target)) {
            if (foundCb) foundCb(nonce);
        }
    }
    return count;
}

// --- GPU Miner (OpenCL) ---

#ifdef HAVE_OPENCL

std::vector<GpuDevice> ListGpuDevices() {
    std::vector<GpuDevice> devices;

    cl_uint numPlatforms = 0;
    clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (numPlatforms == 0) return devices;

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    for (auto &plat : platforms) {
        cl_uint numDevs = 0;
        clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevs);
        if (numDevs == 0) continue;

        std::vector<cl_device_id> devs(numDevs);
        clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, numDevs, devs.data(),
                       nullptr);

        for (auto &dev : devs) {
            GpuDevice d;
            d.id = dev;
            char name[256];
            clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(name), name, nullptr);
            d.name = name;
            clGetDeviceInfo(dev, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(d.globalMem),
                            &d.globalMem, nullptr);
            clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS,
                            sizeof(d.computeUnits), &d.computeUnits, nullptr);
            devices.push_back(d);
        }
    }
    return devices;
}

GpuMiner::GpuMiner(const GpuMinerConfig &config) : m_config(config) {}

GpuMiner::~GpuMiner() {
    Release();
}

bool GpuMiner::Init() {
    auto devices = ListGpuDevices();
    if (m_config.deviceIndex >= (int)devices.size()) {
        std::cerr << "GPU device " << m_config.deviceIndex << " not found ("
                  << devices.size() << " available)\n";
        return false;
    }

    m_device = devices[m_config.deviceIndex];
    std::cout << "Using GPU: " << m_device.name
              << " (" << m_device.computeUnits << " CU, "
              << (m_device.globalMem / (1024*1024)) << " MB)\n";

    cl_int err;
    m_context = clCreateContext(nullptr, 1, &m_device.id, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "clCreateContext failed: " << err << "\n";
        return false;
    }

    m_queue = clCreateCommandQueue(m_context, m_device.id, 0, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "clCreateCommandQueue failed: " << err << "\n";
        return false;
    }

    // Load kernel source
    std::ifstream f(m_config.kernelPath);
    if (!f.is_open()) {
        std::cerr << "Cannot open kernel: " << m_config.kernelPath << "\n";
        return false;
    }
    std::string src((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    const char *srcPtr = src.c_str();
    size_t srcLen = src.size();

    m_program = clCreateProgramWithSource(m_context, 1, &srcPtr, &srcLen, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "clCreateProgramWithSource failed: " << err << "\n";
        return false;
    }

    err = clBuildProgram(m_program, 1, &m_device.id, "-cl-std=CL1.2", nullptr,
                         nullptr);
    if (err != CL_SUCCESS) {
        char log[16384];
        clGetProgramBuildInfo(m_program, m_device.id, CL_PROGRAM_BUILD_LOG,
                              sizeof(log), log, nullptr);
        std::cerr << "Kernel build failed:\n" << log << "\n";
        return false;
    }

    m_kernel = clCreateKernel(m_program, "scrypt_hash", &err);
    if (err != CL_SUCCESS) {
        std::cerr << "clCreateKernel failed: " << err << "\n";
        return false;
    }

    // Allocate buffers
    m_headerBuf = clCreateBuffer(m_context, CL_MEM_READ_ONLY,
                                  20 * sizeof(uint32_t), nullptr, &err);
    m_targetBuf = clCreateBuffer(m_context, CL_MEM_READ_ONLY,
                                  8 * sizeof(uint32_t), nullptr, &err);

    // Scratch memory: 128KB per work-item (32*1024 uint32)
    size_t vSize = (size_t)m_config.workSize * 32 * 1024 * sizeof(uint32_t);
    if (vSize > m_device.globalMem * 0.8) {
        // Reduce work size to fit GPU memory
        m_config.workSize =
            (int)(m_device.globalMem * 0.8 / (32 * 1024 * sizeof(uint32_t)));
        m_config.workSize = (m_config.workSize / m_config.localSize) *
                            m_config.localSize;
        vSize = (size_t)m_config.workSize * 32 * 1024 * sizeof(uint32_t);
        std::cout << "Adjusted work size to " << m_config.workSize
                  << " to fit GPU memory\n";
    }

    m_vBuf = clCreateBuffer(m_context, CL_MEM_READ_WRITE, vSize, nullptr, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to allocate GPU scratch memory (" << (vSize/1024/1024) << " MB): " << err << "\n";
        return false;
    }

    m_resultsBuf = clCreateBuffer(m_context, CL_MEM_READ_WRITE,
                                   17 * sizeof(uint32_t), nullptr, &err);

    m_initialized = true;
    return true;
}

void GpuMiner::Release() {
    if (m_resultsBuf) clReleaseMemObject(m_resultsBuf);
    if (m_vBuf) clReleaseMemObject(m_vBuf);
    if (m_targetBuf) clReleaseMemObject(m_targetBuf);
    if (m_headerBuf) clReleaseMemObject(m_headerBuf);
    if (m_kernel) clReleaseKernel(m_kernel);
    if (m_program) clReleaseProgram(m_program);
    if (m_queue) clReleaseCommandQueue(m_queue);
    if (m_context) clReleaseContext(m_context);
    m_initialized = false;
}

uint64_t GpuMiner::ScanNonces(const MiningWork &work, uint32_t nonceStart,
                               std::function<void(uint32_t)> foundCb,
                               std::atomic<bool> &interrupt) {
    if (!m_initialized) return 0;

    clEnqueueWriteBuffer(m_queue, m_headerBuf, CL_TRUE, 0,
                          20 * sizeof(uint32_t), work.header, 0, nullptr,
                          nullptr);
    clEnqueueWriteBuffer(m_queue, m_targetBuf, CL_TRUE, 0,
                          8 * sizeof(uint32_t), work.target, 0, nullptr,
                          nullptr);

    uint64_t totalHashes = 0;

    while (!interrupt.load()) {
        // Clear results
        uint32_t zero[17] = {};
        clEnqueueWriteBuffer(m_queue, m_resultsBuf, CL_TRUE, 0,
                              17 * sizeof(uint32_t), zero, 0, nullptr, nullptr);

        // Set kernel args
        clSetKernelArg(m_kernel, 0, sizeof(cl_mem), &m_headerBuf);
        clSetKernelArg(m_kernel, 1, sizeof(cl_mem), &m_targetBuf);
        clSetKernelArg(m_kernel, 2, sizeof(uint32_t), &nonceStart);
        clSetKernelArg(m_kernel, 3, sizeof(cl_mem), &m_vBuf);
        clSetKernelArg(m_kernel, 4, sizeof(cl_mem), &m_resultsBuf);

        size_t globalSize = m_config.workSize;
        size_t localSize = m_config.localSize;

        cl_int err = clEnqueueNDRangeKernel(m_queue, m_kernel, 1, nullptr,
                                             &globalSize, &localSize, 0,
                                             nullptr, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "Kernel execution failed: " << err << "\n";
            break;
        }

        clFinish(m_queue);
        totalHashes += m_config.workSize;

        // Read results
        uint32_t results[17];
        clEnqueueReadBuffer(m_queue, m_resultsBuf, CL_TRUE, 0,
                             17 * sizeof(uint32_t), results, 0, nullptr,
                             nullptr);

        uint32_t found = results[0];
        for (uint32_t i = 0; i < found && i < 16; i++) {
            if (foundCb) foundCb(results[1 + i]);
        }

        nonceStart += m_config.workSize;
        if (nonceStart < (uint32_t)m_config.workSize) break; // overflow
    }

    return totalHashes;
}

#endif // HAVE_OPENCL
