#include "string_processing.h"



std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    while (true) {
        auto pos_space = text.find(' ');
        result.push_back(text.substr(0, pos_space));
        if (pos_space == text.npos) {
            break;
        } else {
            text.remove_prefix(pos_space + 1);
        }
    }
    return result;
}
