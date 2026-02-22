#include "ranking.h"
#include "inverted_index.h"

#include <gtest/gtest.h>

using search::InvertedIndex;
using search::TFIDFRanker;

TEST(RankingTest, EmptyIndex) {
    InvertedIndex idx;
    TFIDFRanker   ranker;
    auto results = ranker.rank({"hello"}, idx, 10);
    EXPECT_TRUE(results.empty());
}

TEST(RankingTest, EmptyQuery) {
    InvertedIndex idx;
    idx.addDocument(0, {"hello"});
    TFIDFRanker ranker;
    auto results = ranker.rank({}, idx, 10);
    EXPECT_TRUE(results.empty());
}

TEST(RankingTest, SingleDocSingleTerm) {
    InvertedIndex idx;
    idx.addDocument(0, {"hello", "world"});
    TFIDFRanker ranker;
    auto results = ranker.rank({"hello"}, idx, 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].docId, 0u);
    EXPECT_GT(results[0].score, 0.0);
}

TEST(RankingTest, MoreRelevantDocRankedFirst) {
    InvertedIndex idx;
    // Doc 0 mentions "search" once in a long doc.
    std::vector<std::string> doc0(50, "word");
    doc0[0] = "search";
    idx.addDocument(0, doc0);

    // Doc 1 mentions "search" 5 times in a shorter doc.
    std::vector<std::string> doc1(10, "word");
    for (int i = 0; i < 5; ++i) doc1[i] = "search";
    idx.addDocument(1, doc1);

    TFIDFRanker ranker;
    auto results = ranker.rank({"search"}, idx, 10);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].docId, 1u); // higher TF → ranked first
}

TEST(RankingTest, TopKRespected) {
    InvertedIndex idx;
    for (uint32_t i = 0; i < 20; ++i) {
        idx.addDocument(i, {"common", "term"});
    }
    TFIDFRanker ranker;
    auto results = ranker.rank({"common"}, idx, 5);
    EXPECT_EQ(results.size(), 5u);
}
