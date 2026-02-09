// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// OpenCL Scrypt(1024,1,1) kernel for Dogecoin mining.
// Each work-item computes one Scrypt hash and checks against the target.

// --- SHA-256 primitives (for PBKDF2-SHA256) ---

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

__constant uint sha256_k[64] = {
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
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256_transform(uint *state, const uint *block) {
    uint w[64];
    for (int i = 0; i < 16; i++) w[i] = block[i];
    for (int i = 16; i < 64; i++)
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];

    uint a = state[0], b = state[1], c = state[2], d = state[3];
    uint e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// HMAC-SHA256 for PBKDF2
void hmac_sha256(const uint *key, int key_words,
                 const uint *msg, int msg_words,
                 uint *out) {
    uint ipad[16], opad[16];
    for (int i = 0; i < 16; i++) {
        uint k = (i < key_words) ? key[i] : 0;
        ipad[i] = k ^ 0x36363636;
        opad[i] = k ^ 0x5c5c5c5c;
    }

    // Inner hash: SHA256(ipad || msg)
    uint state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    sha256_transform(state, ipad);

    // Pad message into 64-byte blocks
    uint block[16];
    int total_bits = (msg_words * 4 + 64) * 8;
    for (int i = 0; i < 16; i++)
        block[i] = (i < msg_words) ? msg[i] : 0;
    if (msg_words < 16) {
        // Add 0x80 padding byte
        uint byte_pos = msg_words * 4;
        uint word_pos = msg_words;
        if (word_pos < 16) {
            block[word_pos] = 0x80000000;
        }
        block[14] = 0;
        block[15] = (64 + msg_words * 4) * 8;
        // Byte swap for big-endian SHA
    }
    sha256_transform(state, block);

    // Outer hash: SHA256(opad || inner_hash)
    uint state2[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    sha256_transform(state2, opad);

    uint block2[16];
    for (int i = 0; i < 8; i++) block2[i] = state[i];
    block2[8] = 0x80000000;
    for (int i = 9; i < 15; i++) block2[i] = 0;
    block2[15] = (64 + 32) * 8;
    sha256_transform(state2, block2);

    for (int i = 0; i < 8; i++) out[i] = state2[i];
}

// --- Salsa20/8 core ---

#define R(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

void salsa20_8(uint *B) {
    uint x[16];
    for (int i = 0; i < 16; i++) x[i] = B[i];

    for (int i = 0; i < 8; i += 2) {
        x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
        x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);
        x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
        x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);
        x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
        x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);
        x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
        x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);
        x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
        x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);
        x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
        x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);
        x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
        x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);
        x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
        x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
    }

    for (int i = 0; i < 16; i++) B[i] += x[i];
}

#undef R

// --- Scrypt ROMix (N=1024, r=1, p=1) ---

void scrypt_romix(__private uint *X, __global uint *V) {
    // X is 32 uint (128 bytes = 2 * 64 for r=1)
    for (int i = 0; i < 1024; i++) {
        // V[i] = X
        for (int j = 0; j < 32; j++)
            V[i * 32 + j] = X[j];

        // X = BlockMix(X)
        uint T[16];
        for (int j = 0; j < 16; j++)
            T[j] = X[16 + j]; // last block

        for (int j = 0; j < 16; j++)
            T[j] ^= X[j]; // T = X[0] ^ X[1]
        salsa20_8(T);
        // First half of output
        uint Y0[16];
        for (int j = 0; j < 16; j++) Y0[j] = T[j];

        for (int j = 0; j < 16; j++)
            T[j] ^= X[16 + j];
        salsa20_8(T);
        // Second half
        for (int j = 0; j < 16; j++) X[j] = Y0[j];
        for (int j = 0; j < 16; j++) X[16 + j] = T[j];
    }

    // Mix phase
    for (int i = 0; i < 1024; i++) {
        int idx = X[16] & 1023; // Integerify(X) mod N
        for (int j = 0; j < 32; j++)
            X[j] ^= V[idx * 32 + j];

        // BlockMix
        uint T[16];
        for (int j = 0; j < 16; j++)
            T[j] = X[16 + j];

        for (int j = 0; j < 16; j++)
            T[j] ^= X[j];
        salsa20_8(T);
        uint Y0[16];
        for (int j = 0; j < 16; j++) Y0[j] = T[j];

        for (int j = 0; j < 16; j++)
            T[j] ^= X[16 + j];
        salsa20_8(T);
        for (int j = 0; j < 16; j++) X[j] = Y0[j];
        for (int j = 0; j < 16; j++) X[16 + j] = T[j];
    }
}

// --- Main kernel ---
// header: 20 uint (80 bytes), big-endian
// target: 8 uint (32 bytes), little-endian comparison
// nonce_start: base nonce for this batch
// V_base: scratch memory, 128KB per work-item
// results: [0] = count of found nonces, [1..N] = found nonces

__kernel void scrypt_hash(
    __constant uint *header,     // 80-byte block header (20 words)
    __constant uint *target,     // 32-byte target (8 words, LE)
    uint nonce_start,
    __global uint *V_base,       // scratch: 32*1024 uint per WI
    __global uint *results
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;

    // Copy header and set nonce (word 19 = nonce, little-endian)
    uint hdr[20];
    for (int i = 0; i < 20; i++) hdr[i] = header[i];
    hdr[19] = nonce;

    // PBKDF2-SHA256(header, header, 1, 128) → initial X
    // Simplified: for scrypt(1,1,1), DK length = 128 bytes = 4 blocks of PBKDF2
    uint X[32];

    // Use header as both password and salt for PBKDF2
    // For each block i (1..4), compute HMAC-SHA256(password, salt || INT(i))
    for (int blk = 0; blk < 4; blk++) {
        uint salt_ext[21]; // 80 bytes + 4 bytes block index
        for (int i = 0; i < 20; i++) salt_ext[i] = hdr[i];
        salt_ext[20] = blk + 1; // big-endian block counter
        hmac_sha256(hdr, 20, salt_ext, 21, &X[blk * 8]);
    }

    // ROMix
    __global uint *V = V_base + (gid * 32 * 1024);
    scrypt_romix(X, V);

    // PBKDF2-SHA256(header, X, 1, 32) → final hash
    uint hash[8];
    // Salt = X (32 words = 128 bytes), append block counter 1
    uint salt2[33];
    for (int i = 0; i < 32; i++) salt2[i] = X[i];
    salt2[32] = 1;
    hmac_sha256(hdr, 20, salt2, 33, hash);

    // Compare hash against target (little-endian, MSB first → check from word 7 down)
    bool found = false;
    for (int i = 7; i >= 0; i--) {
        if (hash[i] < target[i]) { found = true; break; }
        if (hash[i] > target[i]) { found = false; break; }
    }

    if (found) {
        uint slot = atomic_inc(&results[0]);
        if (slot < 16) {
            results[1 + slot] = nonce;
        }
    }
}
