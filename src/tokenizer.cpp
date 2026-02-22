#include "tokenizer.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace search {

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string word;
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            word += static_cast<char>(std::tolower(ch));
        } else if (!word.empty()) {
            tokens.push_back(std::move(word));
            word.clear();
        }
    }
    if (!word.empty()) {
        tokens.push_back(std::move(word));
    }
    return tokens;
}

} // namespace search
