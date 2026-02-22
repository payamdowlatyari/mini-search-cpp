#include "tokenizer.h"

#include <gtest/gtest.h>

using search::Tokenizer;

TEST(TokenizerTest, EmptyString) {
    Tokenizer t;
    EXPECT_TRUE(t.tokenize("").empty());
}

TEST(TokenizerTest, Lowercase) {
    Tokenizer t;
    auto toks = t.tokenize("Hello World");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0], "hello");
    EXPECT_EQ(toks[1], "world");
}

TEST(TokenizerTest, PunctuationRemoved) {
    Tokenizer t;
    auto toks = t.tokenize("Hello, World!");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0], "hello");
    EXPECT_EQ(toks[1], "world");
}

TEST(TokenizerTest, Numbers) {
    Tokenizer t;
    auto toks = t.tokenize("C++17 in 2023");
    // "C" "17" "in" "2023"
    EXPECT_EQ(toks.size(), 4u);
}

TEST(TokenizerTest, OnlyPunctuation) {
    Tokenizer t;
    EXPECT_TRUE(t.tokenize("!@#$%^&*()").empty());
}
