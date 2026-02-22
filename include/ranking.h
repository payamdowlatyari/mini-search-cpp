#pragma once

#include "inverted_index.h"

#include <cstdint>
#include <string>
#include <vector>

namespace search {

/// A scored search result.
struct SearchResult {
    uint32_t docId;
    double   score;
};

/// Ranks documents using the TF-IDF weighting scheme.
class TFIDFRanker {
public:
    /// Score and rank documents for all given query terms.
    /// Returns up to `topK` results sorted by descending score.
    std::vector<SearchResult> rank(const std::vector<std::string>& queryTerms,
                                   const InvertedIndex&             index,
                                   size_t                           topK = 10) const;

private:
    /// Term frequency: raw count normalised by document length.
    static double tf(uint32_t termFreq, uint32_t docLength);

    /// Inverse document frequency: log((N + 1) / (df + 1)) + 1  (smoothed).
    static double idf(size_t totalDocs, size_t docFreq);
};

} // namespace search
