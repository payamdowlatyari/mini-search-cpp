#pragma once

#include "inverted_index.h"
#include "ranking.h"
#include "storage.h"
#include "thread_pool.h"
#include "tokenizer.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace search {

/// Rich search result that includes document metadata.
struct FullSearchResult {
    uint32_t    docId;
    std::string title;
    std::string snippet; ///< short excerpt around the matched terms
    double      score;
};

/// High-level search engine that combines all subsystems.
class SearchEngine {
public:
    /// @param dataDir  directory used for persisting data to disk
    /// @param numThreads  worker threads used for concurrent queries
    explicit SearchEngine(const std::string& dataDir, size_t numThreads = 4);

    /// Index a document; returns its assigned document ID.
    uint32_t indexDocument(const std::string& title, const std::string& content);

    /// Search and return up to @p topK results ranked by TF-IDF.
    /// Query processing is dispatched to the internal thread pool.
    std::vector<FullSearchResult> search(const std::string& query, size_t topK = 10);

    /// Remove a document.  Returns false if the document did not exist.
    bool removeDocument(uint32_t docId);

    /// Retrieve a document by ID.
    std::optional<Document> getDocument(uint32_t docId) const;

    /// Persist the current state to disk.
    void save() const;

    /// Engine statistics.
    struct Stats {
        size_t   numDocs;
        size_t   numTerms;
        uint32_t nextDocId;
    };
    Stats getStats() const;

private:
    mutable std::shared_mutex               rwMutex_;
    Tokenizer                               tokenizer_;
    InvertedIndex                           index_;
    TFIDFRanker                             ranker_;
    Storage                                 storage_;
    ThreadPool                              pool_;
    std::unordered_map<uint32_t, Document>  documents_;
    std::atomic<uint32_t>                   nextDocId_{0};

    std::string makeSnippet(const std::string&              content,
                            const std::vector<std::string>& terms,
                            size_t                          maxLen = 200) const;
};

} // namespace search
