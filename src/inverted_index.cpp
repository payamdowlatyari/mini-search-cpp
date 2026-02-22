#include "inverted_index.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace search {

// ---------------------------------------------------------------------------
// helpers: write/read primitive types to a binary stream
// ---------------------------------------------------------------------------
namespace {

template<typename T>
void writePOD(std::ostream& out, T value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
T readPOD(std::istream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

void writeString(std::ostream& out, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    writePOD(out, len);
    out.write(s.data(), len);
}

std::string readString(std::istream& in) {
    uint32_t len = readPOD<uint32_t>(in);
    std::string s(len, '\0');
    in.read(s.data(), len);
    if (!in || static_cast<uint32_t>(in.gcount()) != len) {
        throw std::runtime_error("Unexpected end-of-stream while reading string");
    }
    return s;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// InvertedIndex
// ---------------------------------------------------------------------------

void InvertedIndex::addDocument(uint32_t docId, const std::vector<std::string>& terms) {
    // Count term frequencies locally before taking the write lock.
    std::unordered_map<std::string, uint32_t> freq;
    for (const auto& t : terms) {
        ++freq[t];
    }

    std::unique_lock lock(mutex_);

    // Remove any previous postings for this doc (re-indexing case).
    for (auto& [term, postings] : index_) {
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                           [docId](const Posting& p) { return p.docId == docId; }),
            postings.end());
    }

    for (auto& [term, count] : freq) {
        index_[term].push_back({docId, count});
    }

    docStats_[docId] = {static_cast<uint32_t>(terms.size())};
}

void InvertedIndex::removeDocument(uint32_t docId) {
    std::unique_lock lock(mutex_);
    for (auto& [term, postings] : index_) {
        postings.erase(
            std::remove_if(postings.begin(), postings.end(),
                           [docId](const Posting& p) { return p.docId == docId; }),
            postings.end());
    }
    docStats_.erase(docId);
}

std::vector<Posting> InvertedIndex::getPostings(const std::string& term) const {
    std::shared_lock lock(mutex_);
    auto it = index_.find(term);
    if (it == index_.end()) return {};
    return it->second;
}

DocStats InvertedIndex::getDocStats(uint32_t docId) const {
    std::shared_lock lock(mutex_);
    auto it = docStats_.find(docId);
    if (it == docStats_.end()) {
        throw std::out_of_range("docId not found in index");
    }
    return it->second;
}

size_t InvertedIndex::totalDocs() const {
    std::shared_lock lock(mutex_);
    return docStats_.size();
}

size_t InvertedIndex::docFrequency(const std::string& term) const {
    std::shared_lock lock(mutex_);
    auto it = index_.find(term);
    if (it == index_.end()) return 0;
    return it->second.size();
}

size_t InvertedIndex::totalTerms() const {
    std::shared_lock lock(mutex_);
    return index_.size();
}

void InvertedIndex::serialize(std::ostream& out) const {
    std::shared_lock lock(mutex_);

    // Write doc stats
    writePOD(out, static_cast<uint32_t>(docStats_.size()));
    for (const auto& [docId, stats] : docStats_) {
        writePOD(out, docId);
        writePOD(out, stats.numTerms);
    }

    // Write index
    writePOD(out, static_cast<uint32_t>(index_.size()));
    for (const auto& [term, postings] : index_) {
        writeString(out, term);
        writePOD(out, static_cast<uint32_t>(postings.size()));
        for (const auto& p : postings) {
            writePOD(out, p.docId);
            writePOD(out, p.frequency);
        }
    }
}

void InvertedIndex::deserialize(std::istream& in) {
    std::unique_lock lock(mutex_);
    index_.clear();
    docStats_.clear();

    uint32_t numDocs = readPOD<uint32_t>(in);
    for (uint32_t i = 0; i < numDocs; ++i) {
        uint32_t docId    = readPOD<uint32_t>(in);
        uint32_t numTerms = readPOD<uint32_t>(in);
        docStats_[docId]  = {numTerms};
    }

    uint32_t numTerms = readPOD<uint32_t>(in);
    for (uint32_t i = 0; i < numTerms; ++i) {
        std::string term      = readString(in);
        uint32_t    numPost   = readPOD<uint32_t>(in);
        std::vector<Posting> postings;
        postings.reserve(numPost);
        for (uint32_t j = 0; j < numPost; ++j) {
            Posting p;
            p.docId     = readPOD<uint32_t>(in);
            p.frequency = readPOD<uint32_t>(in);
            postings.push_back(p);
        }
        index_[term] = std::move(postings);
    }
}

void InvertedIndex::clear() {
    std::unique_lock lock(mutex_);
    index_.clear();
    docStats_.clear();
}

} // namespace search
