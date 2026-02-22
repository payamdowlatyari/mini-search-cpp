#include "inverted_index.h"

#include <gtest/gtest.h>
#include <sstream>

using search::InvertedIndex;

TEST(InvertedIndexTest, AddAndGetPostings) {
    InvertedIndex idx;
    idx.addDocument(0, {"hello", "world", "hello"});
    auto postings = idx.getPostings("hello");
    ASSERT_EQ(postings.size(), 1u);
    EXPECT_EQ(postings[0].docId, 0u);
    EXPECT_EQ(postings[0].frequency, 2u);
}

TEST(InvertedIndexTest, MissingTermReturnsEmpty) {
    InvertedIndex idx;
    EXPECT_TRUE(idx.getPostings("absent").empty());
}

TEST(InvertedIndexTest, TotalDocs) {
    InvertedIndex idx;
    EXPECT_EQ(idx.totalDocs(), 0u);
    idx.addDocument(0, {"a", "b"});
    EXPECT_EQ(idx.totalDocs(), 1u);
    idx.addDocument(1, {"c"});
    EXPECT_EQ(idx.totalDocs(), 2u);
}

TEST(InvertedIndexTest, DocFrequency) {
    InvertedIndex idx;
    idx.addDocument(0, {"cat", "dog"});
    idx.addDocument(1, {"cat"});
    EXPECT_EQ(idx.docFrequency("cat"), 2u);
    EXPECT_EQ(idx.docFrequency("dog"), 1u);
}

TEST(InvertedIndexTest, RemoveDocument) {
    InvertedIndex idx;
    idx.addDocument(0, {"hello"});
    idx.addDocument(1, {"hello"});
    idx.removeDocument(0);
    EXPECT_EQ(idx.totalDocs(), 1u);
    auto postings = idx.getPostings("hello");
    ASSERT_EQ(postings.size(), 1u);
    EXPECT_EQ(postings[0].docId, 1u);
}

TEST(InvertedIndexTest, SerializeDeserialize) {
    InvertedIndex idx;
    idx.addDocument(0, {"hello", "world"});
    idx.addDocument(1, {"hello"});

    std::ostringstream oss;
    idx.serialize(oss);

    InvertedIndex idx2;
    std::istringstream iss(oss.str());
    idx2.deserialize(iss);

    EXPECT_EQ(idx2.totalDocs(), 2u);
    EXPECT_EQ(idx2.docFrequency("hello"), 2u);
    EXPECT_EQ(idx2.docFrequency("world"), 1u);
}
