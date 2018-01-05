//
// Created by gros on 14.12.17.
//

#include "Helpers.h"
#include "Function.h"

bool startswith(std::string s, std::string beginning) {
    return s.size() >= beginning.size() &&
            s.compare(0, beginning.size(), beginning) == 0;
}

bool endswith(std::string s, std::string ending) {
    return s.size() >= ending.size() &&
            s.compare(s.size() - ending.size(), ending.size(), ending) == 0;
}

bool contains(std::string haystack, std::string needle) {
    return haystack.find(needle) != haystack.npos;
}

std::string vector2string(std::vector<std::set<char>> v) {
    std::string result = "[";
    for (auto &&s : v) {
        result += "{";
        for (auto &&x : s)
            result += const2str(x) + "|";
        result += "}, ";
    }
    result += "]";
    return result;
}