/// mini-search-cpp – lightweight search engine with a REST API (Crow).
///
/// Endpoints:
///   POST   /api/documents          – index a document  {title, content}
///   GET    /api/documents/<id>     – retrieve a document by ID
///   DELETE /api/documents/<id>     – remove a document
///   GET    /api/search?q=…&limit=N – search (default limit = 10)
///   POST   /api/save               – persist the index to disk
///   GET    /api/stats              – engine statistics

#include "search_engine.h"

#include <crow.h>

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Configuration from environment / command-line.
    std::string dataDir   = (argc > 1) ? argv[1] : "data";
    uint16_t    port      = 8080;
    size_t      threads   = 4;

    if (const char* p = std::getenv("SEARCH_PORT"))    port    = static_cast<uint16_t>(std::stoi(p));
    if (const char* t = std::getenv("SEARCH_THREADS")) threads = static_cast<size_t>(std::stoi(t));
    if (const char* d = std::getenv("SEARCH_DATA_DIR")) dataDir = d;

    search::SearchEngine engine(dataDir, threads);

    crow::SimpleApp app;

    // -----------------------------------------------------------------------
    // POST /api/documents  – index a new document
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/documents").methods(crow::HTTPMethod::POST)(
        [&engine](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("title") || !body.has("content")) {
                return crow::response(400, R"({"error":"Missing 'title' or 'content'"})");
            }
            std::string title   = body["title"].s();
            std::string content = body["content"].s();

            uint32_t docId = engine.indexDocument(title, content);

            crow::json::wvalue resp;
            resp["id"]      = docId;
            resp["message"] = "Document indexed successfully";
            return crow::response(201, resp);
        });

    // -----------------------------------------------------------------------
    // GET /api/documents/<id>  – retrieve a document
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/documents/<uint>").methods(crow::HTTPMethod::GET)(
        [&engine](uint32_t docId) {
            auto doc = engine.getDocument(docId);
            if (!doc) {
                return crow::response(404, R"({"error":"Document not found"})");
            }
            crow::json::wvalue resp;
            resp["id"]      = doc->id;
            resp["title"]   = doc->title;
            resp["content"] = doc->content;
            return crow::response(200, resp);
        });

    // -----------------------------------------------------------------------
    // DELETE /api/documents/<id>  – remove a document
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/documents/<uint>").methods(crow::HTTPMethod::DELETE)(
        [&engine](uint32_t docId) {
            if (!engine.removeDocument(docId)) {
                return crow::response(404, R"({"error":"Document not found"})");
            }
            return crow::response(200, R"({"message":"Document removed"})");
        });

    // -----------------------------------------------------------------------
    // GET /api/search?q=…&limit=N  – search
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/search").methods(crow::HTTPMethod::GET)(
        [&engine](const crow::request& req) {
            std::string query = req.url_params.get("q") ? req.url_params.get("q") : "";
            if (query.empty()) {
                return crow::response(400, R"({"error":"Missing query parameter 'q'"})");
            }

            size_t limit = 10;
            if (req.url_params.get("limit")) {
                try {
                    int l = std::stoi(req.url_params.get("limit"));
                    if (l > 0) limit = static_cast<size_t>(l);
                } catch (...) {}
            }

            auto results = engine.search(query, limit);

            crow::json::wvalue resp;
            resp["query"] = query;
            resp["total"] = static_cast<int>(results.size());

            std::vector<crow::json::wvalue> hits;
            hits.reserve(results.size());
            for (const auto& r : results) {
                crow::json::wvalue hit;
                hit["id"]      = r.docId;
                hit["title"]   = r.title;
                hit["snippet"] = r.snippet;
                hit["score"]   = r.score;
                hits.emplace_back(std::move(hit));
            }
            resp["hits"] = std::move(hits);
            return crow::response(200, resp);
        });

    // -----------------------------------------------------------------------
    // POST /api/save  – persist index to disk
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/save").methods(crow::HTTPMethod::POST)(
        [&engine]() {
            try {
                engine.save();
                return crow::response(200, R"({"message":"Saved"})");
            } catch (const std::exception& e) {
                return crow::response(500, std::string(R"({"error":")") + e.what() + "\"}");
            }
        });

    // -----------------------------------------------------------------------
    // GET /api/stats
    // -----------------------------------------------------------------------
    CROW_ROUTE(app, "/api/stats").methods(crow::HTTPMethod::GET)(
        [&engine]() {
            auto s = engine.getStats();
            crow::json::wvalue resp;
            resp["numDocs"]   = static_cast<int>(s.numDocs);
            resp["numTerms"]  = static_cast<int>(s.numTerms);
            resp["nextDocId"] = static_cast<int>(s.nextDocId);
            return crow::response(200, resp);
        });

    std::cout << "mini-search listening on port " << port
              << " (data: " << dataDir << ")\n";
    app.port(port).multithreaded().run();
    return 0;
}
