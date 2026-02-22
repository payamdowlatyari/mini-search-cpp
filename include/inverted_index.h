#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <iosfwd>

namespace search {

/// A single entry in a postings list.
struct Posting {
    uint32_t docId;
    uint32_t frequency; ///< how many times the term appears in the document
};

/// Per-document metadata stored alongside the index.
struct DocStats {
    uint32_t numTerms; ///< total number of tokens in the document
};

/// Thread-safe inverted index mapping terms to their postings lists.
class InvertedIndex {
public:
    /// Add (or update) a document given its token list.
    void addDocument(uint32_t docId, const std::vector<std::string>& terms);

    /// Remove all postings for the given document.
    void removeDocument(uint32_t docId);

    /// Return a copy of the postings list for the given term (empty if absent).
    std::vector<Posting> getPostings(const std::string& term) const;

    /// Return the DocStats for the given document (throws if absent).
    DocStats getDocStats(uint32_t docId) const;

    /// Total number of indexed documents.
    size_t totalDocs() const;

    /// Number of documents that contain the given term (document frequency).
    size_t docFrequency(const std::string& term) const;

    /// Total number of distinct terms in the index.
    size_t totalTerms() const;

    /// Serialize the index to a binary stream.
    void serialize(std::ostream& out) const;

    /// Deserialize the index from a binary stream (replaces current state).
    void deserialize(std::istream& in);

    /// Clear all data.
    void clear();

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::vector<Posting>> index_;
    std::unordered_map<uint32_t, DocStats> docStats_;
};

} // namespace search
