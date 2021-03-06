#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    const int64_t pos_end = text.npos;
    while (true) {
        int64_t space = text.find(' ', 0);
        if (space!=0){
        words.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));}
        if (space == pos_end) {
            break;
        } else {
            text.remove_prefix(space+1);  
        }
    }
    return words;
}
