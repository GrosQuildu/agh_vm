//
// Created by gros on 14.12.17.
//

#ifndef VM_HELPERS_H
#define VM_HELPERS_H

#include <string>
#include <algorithm>

bool startswith(std::string, std::string);
bool endswith(std::string, std::string);
bool contains(std::string, std::string);

// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

#endif //VM_HELPERS_H
