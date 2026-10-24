#include <algorithm>
#include <cctype>
#include <string>

#include "text.h"

namespace nano_edr {

namespace {

bool CharEqualsIgoneCase (char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

}

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    return std::ranges::equal(a, b, CharEqualsIgoneCase);
}

bool ContainsIgnoreCase(const std::string& text, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > text.size()) {
        return false;
    }
    auto it = std::ranges::search(text, needle, CharEqualsIgoneCase);
    return !it.empty();
}

bool EndsWithIgnoreCase(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    return std::equal(text.end() - suffix.size(), text.end(),
     suffix.begin(), suffix.end(),
      CharEqualsIgoneCase);
}   

} // namespace nano_edr
