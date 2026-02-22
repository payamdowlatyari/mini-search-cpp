# mini-search-cpp

A lightweight, single-node full-text search engine written in modern C++17.

## Features

| Component | Description |
|---|---|
| **Tokenizer** | Splits text into lowercase alphanumeric tokens |
| **Inverted Index** | Maps every term to its postings list (docId + frequency) |
| **TF-IDF Ranking** | Scores and ranks documents using smoothed TF-IDF |
| **Thread Pool** | Fixed-size pool dispatches concurrent query tasks |
| **Persistence** | Atomic binary serialisation of documents and index to disk |
| **REST API** | JSON HTTP API powered by [Crow](https://github.com/CrowCpp/Crow) |

## Architecture

```
src/
├── tokenizer.cpp        – text → lowercase alphanumeric tokens
├── inverted_index.cpp   – thread-safe index (std::shared_mutex)
├── ranking.cpp          – TF-IDF scoring with partial_sort for top-K
├── storage.cpp          – atomic binary persistence to disk
├── thread_pool.cpp      – std::thread-based fixed-size worker pool
├── search_engine.cpp    – façade combining all subsystems
└── main.cpp             – Crow HTTP server (REST endpoints)

include/
├── tokenizer.h
├── inverted_index.h
├── ranking.h
├── storage.h
├── thread_pool.h
└── search_engine.h

tests/
├── test_tokenizer.cpp
├── test_inverted_index.cpp
├── test_ranking.cpp
└── test_search_engine.cpp
```

## REST API

| Method | Path | Body / Params | Description |
|--------|------|---------------|-------------|
| `POST` | `/api/documents` | `{"title":"…","content":"…"}` | Index a document; returns its `id` |
| `GET` | `/api/documents/<id>` | – | Retrieve a document by ID |
| `DELETE` | `/api/documents/<id>` | – | Remove a document |
| `GET` | `/api/search?q=…&limit=N` | – | Search (default limit = 10) |
| `POST` | `/api/save` | – | Flush index and documents to disk |
| `GET` | `/api/stats` | – | Engine statistics |

### Examples

```bash
# Index a document
curl -X POST http://localhost:8080/api/documents \
     -H 'Content-Type: application/json' \
     -d '{"title":"C++ Primer","content":"A comprehensive guide to modern C++"}'
# → {"id":0,"message":"Document indexed successfully"}

# Search
curl 'http://localhost:8080/api/search?q=modern+cpp&limit=5'
# → {"query":"modern cpp","total":1,"hits":[{"id":0,"title":"C++ Primer","snippet":"…","score":0.25}]}

# Persist to disk
curl -X POST http://localhost:8080/api/save
```

## Build

### Requirements
- CMake ≥ 3.15
- GCC / Clang with C++17 support
- Internet access at configure time (FetchContent downloads Crow, Asio, GoogleTest)

### Steps

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The main binary is `build/mini-search`.

### Tests

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Configuration

| Environment Variable | Default | Description |
|---|---|---|
| `SEARCH_PORT` | `8080` | HTTP port |
| `SEARCH_THREADS` | `4` | Thread pool size |
| `SEARCH_DATA_DIR` | `data` | Persistence directory |

You can also pass the data directory as the first command-line argument:

```bash
./mini-search /var/lib/mini-search
```

## Tech Stack

- **C++17** – STL containers, `<filesystem>`, structured bindings
- **std::shared_mutex** – concurrent readers / exclusive writers on the index
- **std::thread + std::packaged_task** – thread pool for parallel query execution
- **Binary file I/O** – compact, fast persistence with atomic rename
- **[Crow](https://github.com/CrowCpp/Crow)** – header-only HTTP/REST framework
- **[Asio](https://think-async.com/Asio/)** – standalone async I/O used by Crow
