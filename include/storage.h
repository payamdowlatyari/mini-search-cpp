#pragma once

#include "inverted_index.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace search {

/// A stored document.
struct Document {
    uint32_t    id;
    std::string title;
    std::string content;
};

/// Persists documents and the inverted index to/from a data directory on disk.
class Storage {
public:
    explicit Storage(std::string dataDir);

    /// Write documents and index to disk (atomic: writes to temp file then renames).
    void save(const std::unordered_map<uint32_t, Document>& docs,
              const InvertedIndex&                           index,
              uint32_t                                       nextDocId) const;

    /// Read documents and index from disk.  Returns false if no data exists yet.
    bool load(std::unordered_map<uint32_t, Document>& docs,
              InvertedIndex&                           index,
              uint32_t&                                nextDocId) const;

private:
    std::string dataDir_;

    std::string docsPath()  const;
    std::string indexPath() const;

    void saveDocuments(const std::unordered_map<uint32_t, Document>& docs,
                       uint32_t nextDocId) const;
    bool loadDocuments(std::unordered_map<uint32_t, Document>& docs,
                       uint32_t&                                nextDocId) const;
};

} // namespace search
