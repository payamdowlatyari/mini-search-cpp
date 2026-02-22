#include "search_engine.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class SearchEngineTest : public ::testing::Test {
protected:
    std::string dataDir;

    void SetUp() override {
        dataDir = (fs::temp_directory_path() / "mini_search_test").string();
        fs::remove_all(dataDir);
    }
    void TearDown() override {
        fs::remove_all(dataDir);
    }
};

TEST_F(SearchEngineTest, IndexAndSearch) {
    search::SearchEngine engine(dataDir, 2);

    uint32_t id0 = engine.indexDocument("C++ Primer", "A comprehensive guide to modern C++");
    engine.indexDocument("Python Guide", "Learn Python programming quickly");

    auto results = engine.search("c++ guide", 10);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].docId, id0);
}

TEST_F(SearchEngineTest, GetDocument) {
    search::SearchEngine engine(dataDir, 2);
    uint32_t id = engine.indexDocument("Test", "Hello world");
    auto doc = engine.getDocument(id);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->title, "Test");
}

TEST_F(SearchEngineTest, RemoveDocument) {
    search::SearchEngine engine(dataDir, 2);
    uint32_t id = engine.indexDocument("Title", "content");
    EXPECT_TRUE(engine.removeDocument(id));
    EXPECT_FALSE(engine.removeDocument(id)); // already removed
    EXPECT_FALSE(engine.getDocument(id).has_value());
}

TEST_F(SearchEngineTest, Stats) {
    search::SearchEngine engine(dataDir, 2);
    EXPECT_EQ(engine.getStats().numDocs, 0u);
    engine.indexDocument("A", "alpha beta");
    engine.indexDocument("B", "gamma delta");
    EXPECT_EQ(engine.getStats().numDocs, 2u);
}

TEST_F(SearchEngineTest, PersistAndReload) {
    {
        search::SearchEngine engine(dataDir, 2);
        engine.indexDocument("Persistent Doc", "data that should survive a restart");
        engine.save();
    }
    // Reload
    search::SearchEngine engine2(dataDir, 2);
    EXPECT_EQ(engine2.getStats().numDocs, 1u);
    auto results = engine2.search("survive", 5);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].title, "Persistent Doc");
}
