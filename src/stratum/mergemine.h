// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_MERGEMINE_H
#define BITCOIN_STRATUM_MERGEMINE_H

#include <sync.h>
#include <uint256.h>
#include <univalue.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace stratum {

/**
 * Configuration for one external chain whose merge-mining is driven via
 * JSON-RPC to a running full node (e.g. litecoind, dogecoind).
 */
struct ExternalChainConfig {
    std::string name;      // human label, e.g. "LTC"
    std::string rpcHost;   // e.g. "127.0.0.1"
    uint16_t rpcPort;      // e.g. 9332
    std::string rpcUser;
    std::string rpcPassword;
    uint32_t chainId;      // AuxPoW chain ID (0x62=DOGE, 2=LTC, ...)
    int pollIntervalMs;    // how often to refresh work (default 5000)
};

/**
 * A snapshot of merge-mine work obtained from an external chain via
 * its createauxblock (or getauxblock) RPC.
 */
struct ExternalAuxWork {
    std::string chainName;
    uint32_t chainId;
    uint256 auxHash;     // the hash the parent coinbase must commit to
    uint32_t nBits;
    int32_t height;
    std::string target;  // hex
    int64_t fetchedAt;   // GetTime() when last refreshed
};

/**
 * Result of submitting solved work back to an external chain.
 */
struct ExternalSubmitResult {
    bool accepted;
    std::string error;
};

/**
 * Abstract interface for an external merge-mined chain.
 * Implementations handle polling for work and submitting solved blocks.
 * The default RPC-based implementation is ExternalChainClient below.
 */
class IAuxChainModule {
public:
    virtual ~IAuxChainModule() = default;

    virtual std::string Name() const = 0;
    virtual uint32_t ChainId() const = 0;
    virtual bool RefreshWork(const std::string &payoutAddr) = 0;
    virtual ExternalAuxWork GetCurrentWork() const = 0;
    virtual ExternalSubmitResult SubmitAuxBlock(const uint256 &hash,
                                               const std::string &auxpowHex) = 0;
    virtual int PollIntervalMs() const = 0;
};

/**
 * Default JSON-RPC client implementation of IAuxChainModule.
 * Connects to a running full node (e.g. litecoind, dogecoind) via RPC.
 */
class ExternalChainClient : public IAuxChainModule {
public:
    explicit ExternalChainClient(const ExternalChainConfig &cfg);
    ~ExternalChainClient() override;

    std::string Name() const override { return m_cfg.name; }
    uint32_t ChainId() const override { return m_cfg.chainId; }
    bool RefreshWork(const std::string &address) override;
    ExternalSubmitResult SubmitAuxBlock(const uint256 &hash,
                                       const std::string &auxpowHex) override;
    ExternalAuxWork GetCurrentWork() const override;
    int PollIntervalMs() const override { return m_cfg.pollIntervalMs; }

    const ExternalChainConfig &GetConfig() const { return m_cfg; }

private:
    UniValue CallRPC(const std::string &method, const UniValue &params);

    ExternalChainConfig m_cfg;
    mutable Mutex m_mutex;
    ExternalAuxWork m_currentWork GUARDED_BY(m_mutex);
};

/**
 * Manages multiple external chains for multi-chain merged mining.
 * Polls each chain periodically and provides a combined set of aux hashes
 * for building the merge-mine commitment tree.
 */
class MergeMineManager {
public:
    MergeMineManager();
    ~MergeMineManager();

    void AddChain(const ExternalChainConfig &cfg);

    /** Register a custom chain module (for non-RPC implementations). */
    void AddModule(std::unique_ptr<IAuxChainModule> module);

    /** Start background polling threads for all chains. */
    void Start(const std::string &payoutAddress);

    /** Interrupt and stop all background threads. */
    void Stop();

    /**
     * Get all current external aux work items. The returned map is
     * keyed by chain name.
     */
    std::map<std::string, ExternalAuxWork> GetAllWork() const;

    /**
     * Collect all external aux hashes, sorted by chainId for deterministic
     * tree placement.
     */
    std::vector<uint256> GetAuxHashes() const;

    /**
     * Get all current work items as a vector sorted by chainId.
     * Used by stratum job creation to snapshot targets alongside hashes.
     */
    std::vector<ExternalAuxWork> GetAllWorkSorted() const;

    /**
     * Submit solved work to a specific external chain by name.
     */
    ExternalSubmitResult SubmitToChain(const std::string &chainName,
                                      const uint256 &hash,
                                      const std::string &auxpowHex);

    /** Submit solved work to ALL external chains that match the target. */
    std::map<std::string, ExternalSubmitResult>
    SubmitToAll(const std::string &auxpowHex);

    size_t ChainCount() const;

    using WorkCallback = std::function<void(const std::string &chainName,
                                            const ExternalAuxWork &work)>;
    void SetWorkCallback(WorkCallback cb);

private:
    void PollLoop(size_t chainIndex);

    mutable Mutex m_mutex;
    std::vector<std::unique_ptr<IAuxChainModule>> m_chains
        GUARDED_BY(m_mutex);
    std::vector<std::thread> m_pollThreads;
    std::atomic<bool> m_running{false};
    std::string m_payoutAddress;
    WorkCallback m_workCallback;
};

// Global lifecycle
void InitMergeMineManager();
MergeMineManager *GetMergeMineManager();
void StopMergeMineManager();

} // namespace stratum

#endif // BITCOIN_STRATUM_MERGEMINE_H
