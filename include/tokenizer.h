#pragma once

#include <string>
#include <vector>

namespace search {

/// Tokenizes text into lowercase terms by stripping punctuation and splitting on whitespace.
class Tokenizer {
public:
    /// Returns a list of normalized tokens from the given text.
    std::vector<std::string> tokenize(const std::string& text) const;
};

} // namespace search
