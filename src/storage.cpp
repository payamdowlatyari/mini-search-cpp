#include "storage.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace search {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
namespace {

template<typename T>
void writePOD(std::ostream& out, T v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

template<typename T>
T readPOD(std::istream& in) {
    T v{};
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

void writeStr(std::ostream& out, const std::string& s) {
    writePOD(out, static_cast<uint32_t>(s.size()));
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}

std::string readStr(std::istream& in) {
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
// Storage
// ---------------------------------------------------------------------------

Storage::Storage(std::string dataDir) : dataDir_(std::move(dataDir)) {
    fs::create_directories(dataDir_);
}

std::string Storage::docsPath()  const { return dataDir_ + "/documents.bin"; }
std::string Storage::indexPath() const { return dataDir_ + "/index.bin"; }

void Storage::save(const std::unordered_map<uint32_t, Document>& docs,
                   const InvertedIndex&                           index,
                   uint32_t                                       nextDocId) const {
    saveDocuments(docs, nextDocId);

    // Save index to a temp file then rename for atomicity.
    std::string tmp = indexPath() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot open index file for writing: " + tmp);
        index.serialize(out);
    }
    fs::rename(tmp, indexPath());
}

bool Storage::load(std::unordered_map<uint32_t, Document>& docs,
                   InvertedIndex&                           index,
                   uint32_t&                                nextDocId) const {
    if (!loadDocuments(docs, nextDocId)) return false;

    std::ifstream in(indexPath(), std::ios::binary);
    if (!in) return false;
    index.deserialize(in);
    return true;
}

void Storage::saveDocuments(const std::unordered_map<uint32_t, Document>& docs,
                            uint32_t nextDocId) const {
    std::string tmp = docsPath() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot open docs file for writing: " + tmp);

        writePOD(out, nextDocId);
        writePOD(out, static_cast<uint32_t>(docs.size()));
        for (const auto& [id, doc] : docs) {
            writePOD(out, doc.id);
            writeStr(out, doc.title);
            writeStr(out, doc.content);
        }
    }
    fs::rename(tmp, docsPath());
}

bool Storage::loadDocuments(std::unordered_map<uint32_t, Document>& docs,
                            uint32_t&                                nextDocId) const {
    std::ifstream in(docsPath(), std::ios::binary);
    if (!in) return false;

    nextDocId        = readPOD<uint32_t>(in);
    uint32_t numDocs = readPOD<uint32_t>(in);
    docs.clear();
    docs.reserve(numDocs);
    for (uint32_t i = 0; i < numDocs; ++i) {
        Document doc;
        doc.id      = readPOD<uint32_t>(in);
        doc.title   = readStr(in);
        doc.content = readStr(in);
        docs[doc.id] = std::move(doc);
    }
    return true;
}

} // namespace search
