//
// Created by gros on 14.12.17.
//

#include "Helpers.h"

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

