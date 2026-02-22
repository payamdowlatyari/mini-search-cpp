#include "ranking.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace search {

double TFIDFRanker::tf(uint32_t termFreq, uint32_t docLength) {
    if (docLength == 0) return 0.0;
    return static_cast<double>(termFreq) / static_cast<double>(docLength);
}

double TFIDFRanker::idf(size_t totalDocs, size_t docFreq) {
    // Smoothed IDF to avoid division by zero.
    return std::log((static_cast<double>(totalDocs) + 1.0) /
                    (static_cast<double>(docFreq)   + 1.0)) + 1.0;
}

std::vector<SearchResult> TFIDFRanker::rank(const std::vector<std::string>& queryTerms,
                                            const InvertedIndex&             index,
                                            size_t                           topK) const {
    size_t N = index.totalDocs();
    if (N == 0 || queryTerms.empty()) return {};

    // Accumulate TF-IDF scores per document.
    std::unordered_map<uint32_t, double> scores;

    for (const auto& term : queryTerms) {
        auto postings = index.getPostings(term);
        if (postings.empty()) continue;

        size_t df      = postings.size();
        double idfVal  = idf(N, df);

        for (const auto& p : postings) {
            DocStats stats{};
            try {
                stats = index.getDocStats(p.docId);
            } catch (...) {
                continue;
            }
            double tfVal = tf(p.frequency, stats.numTerms);
            scores[p.docId] += tfVal * idfVal;
        }
    }

    // Collect and sort.
    std::vector<SearchResult> results;
    results.reserve(scores.size());
    for (auto& [docId, score] : scores) {
        results.push_back({docId, score});
    }

    size_t k = std::min(topK, results.size());
    std::partial_sort(results.begin(), results.begin() + static_cast<std::ptrdiff_t>(k),
                      results.end(),
                      [](const SearchResult& a, const SearchResult& b) {
                          return a.score > b.score;
                      });
    results.resize(k);
    return results;
}

} // namespace search
