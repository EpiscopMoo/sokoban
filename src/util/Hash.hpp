#pragma once
#include <cstddef>
#include <functional>

#define HASH_SUPPORT(clazz) namespace std { template<> struct hash<clazz> { \
    inline size_t operator()(const clazz& x) const { return x.hash(); } \
}; }

// https://stackoverflow.com/a/19195373
template <class T>
inline size_t hash_combine(size_t s, const T& v) {
    static std::hash<T> h;
    return s ^ (h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2));
}