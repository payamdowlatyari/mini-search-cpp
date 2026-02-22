#include "search_engine.h"

#include <algorithm>
#include <cctype>

namespace search {

SearchEngine::SearchEngine(const std::string& dataDir, size_t numThreads)
    : storage_(dataDir), pool_(numThreads) {
    // Attempt to restore from disk.
    uint32_t savedNextId = 0;
    if (storage_.load(documents_, index_, savedNextId)) {
        nextDocId_.store(savedNextId);
    }
}

uint32_t SearchEngine::indexDocument(const std::string& title,
                                     const std::string& content) {
    auto tokens = tokenizer_.tokenize(title + " " + content);
    uint32_t docId = nextDocId_.fetch_add(1);

    {
        std::unique_lock lock(rwMutex_);
        documents_[docId] = {docId, title, content};
        index_.addDocument(docId, tokens);
    }
    return docId;
}

std::vector<FullSearchResult> SearchEngine::search(const std::string& query,
                                                   size_t topK) {
    auto terms = tokenizer_.tokenize(query);

    // Dispatch ranking to a pool thread so concurrent requests don't block.
    auto future = pool_.submit([this, terms, topK]() {
        std::shared_lock lock(rwMutex_);
        return ranker_.rank(terms, index_, topK);
    });

    auto rawResults = future.get();

    std::vector<FullSearchResult> results;
    results.reserve(rawResults.size());

    std::shared_lock lock(rwMutex_);
    for (const auto& r : rawResults) {
        auto it = documents_.find(r.docId);
        if (it == documents_.end()) continue;
        const auto& doc = it->second;
        results.push_back({r.docId, doc.title, makeSnippet(doc.content, terms), r.score});
    }
    return results;
}

bool SearchEngine::removeDocument(uint32_t docId) {
    std::unique_lock lock(rwMutex_);
    if (documents_.find(docId) == documents_.end()) return false;
    documents_.erase(docId);
    index_.removeDocument(docId);
    return true;
}

std::optional<Document> SearchEngine::getDocument(uint32_t docId) const {
    std::shared_lock lock(rwMutex_);
    auto it = documents_.find(docId);
    if (it == documents_.end()) return std::nullopt;
    return it->second;
}

void SearchEngine::save() const {
    std::shared_lock lock(rwMutex_);
    storage_.save(documents_, index_, nextDocId_.load());
}

SearchEngine::Stats SearchEngine::getStats() const {
    std::shared_lock lock(rwMutex_);
    return {documents_.size(), index_.totalTerms(), nextDocId_.load()};
}

std::string SearchEngine::makeSnippet(const std::string&              content,
                                      const std::vector<std::string>& terms,
                                      size_t                          maxLen) const {
    if (content.empty()) return {};

    // Find the earliest occurrence of any query term (case-insensitive).
    std::string lower = content;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    size_t bestPos = std::string::npos;
    for (const auto& term : terms) {
        size_t pos = lower.find(term);
        if (pos != std::string::npos &&
            (bestPos == std::string::npos || pos < bestPos)) {
            bestPos = pos;
        }
    }

    size_t start = 0;
    if (bestPos != std::string::npos && bestPos > 40) {
        start = bestPos - 40;
    }

    std::string snippet = content.substr(start, maxLen);
    if (start > 0)            snippet = "..." + snippet;
    if (start + maxLen < content.size()) snippet += "...";
    return snippet;
}

} // namespace search
