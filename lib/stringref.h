#ifndef P4C_LIB_STRINGREF_H_
#define P4C_LIB_STRINGREF_H_

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include "cstring.h"

/* A StringRef is a *TEMPORARY* reference to a string held in memory managed by some other
 * object.  As such, it becomes dangling (invalid) when that other object goes away, so needs
 * to be used with care.  StringRefs should in general have short lifetimes, and not be
 * stored in other long-lived objects. */

struct StringRef {
    const char  *p;
    size_t      len;
    StringRef() : p(0), len(0) {}
    StringRef(const char *str, size_t l) : p(str), len(l) {}
    StringRef(const char *str) : p(str), len(str ? strlen(str) : 0) {}     // NOLINT
    StringRef(const std::string &str) : p(str.data()), len(str.size()) {}  // NOLINT
    StringRef(cstring str) : p(str.c_str()), len(str.size()) {}            // NOLINT
    void clear() { p = 0; len = 0; }
    StringRef(const StringRef &a) : p(a.p), len(a.len) {}
    StringRef &operator=(const StringRef &a) {
        p = a.p; len = a.len; return *this; }
    explicit operator bool () const { return p != 0; }

    bool operator==(const StringRef &a) const {
        return p ? (a.p && len == a.len && (!len || !memcmp(p, a.p, len))) : !a.p; }
    bool operator!=(const StringRef &a) const { return !operator==(a); }
    bool operator==(const std::string &a) const { return operator==(StringRef(a)); }
    bool operator==(const char *a) const {
        return p ? (a && (!len || !strncmp(p, a, len)) && !a[len]) : !a; }
    bool operator==(cstring a) const { return operator==(a.c_str()); }
    template <class T> bool operator!=(T a) const { return !(*this == a); }
    bool isNullOrEmpty() const { return p == 0 || len == 0; }

    int compare(const StringRef &a) const {
        if (!p) return a.p ? -1 : 0;
        if (!a.p) return 1;
        int rv = memcmp(p, a.p, std::min(len, a.len));
        if (!rv && len != a.len) rv = len < a.len ? -1 : 1;
        return rv; }
    int compare(const std::string &a) const { return compare(StringRef(a)); }
    int compare(const char *a) const {
        if (!p) return a ? -1 : 0;
        if (!a) return 1;
        int rv = strncmp(p, a, len);
        if (!rv && a[len]) rv = -1;
        return rv; }
    int compare(cstring a) const { return compare(a.c_str()); }
    template <class T> bool operator<(T a) const { return compare(a) < 0; }
    template <class T> bool operator<=(T a) const { return compare(a) <= 0; }
    template <class T> bool operator>(T a) const { return compare(a) > 0; }
    template <class T> bool operator>=(T a) const { return compare(a) >= 0; }

    operator std::string() const { return std::string(p, len); }
    operator cstring() const { return std::string(p, len); }
    cstring toString() const { return std::string(p, len); }
    std::string string() const { return std::string(p, len); }
    StringRef &operator+=(size_t i) {
        if (len < i) { p = 0; len = 0;
        } else { p += i; len -= i; }
        return *this; }
    StringRef &operator++() { p++; if (len) len--; else p = 0; return *this; }  // NOLINT
    StringRef operator++(int) { StringRef rv(*this); ++*this; return rv; }
    char operator[](size_t i) const { return i < len ? p[i] : 0; }
    char operator*() const { return len ? *p : 0; }
    StringRef operator+(size_t i) const {
        StringRef rv(*this); rv += i; return rv; }
    StringRef &trim(const char *white = " \t\r\n") {
        while (len > 0 && strchr(white, *p)) { p++; len--; }
        while (len > 0 && strchr(white, p[len-1])) {len--; }
        return *this; }
    bool trimCR() {
        bool rv = false;
        while (len > 0 && p[len-1] == '\r') { rv = true; len--; }
        return rv; }
    StringRef trim(const char *white = " \t\r\n") const {
        StringRef rv(*this);
        rv.trim(white);
        return rv; }
    const char *begin() const { return p; }
    const char *end() const { return p + len; }
    const char *find(char ch) const {
        return p ? static_cast<const char *>(memchr(p, ch, len)) : p; }
    const char *findlast(char ch) const {
        return p ? static_cast<const char *>(memrchr(p, ch, len)) : p; }
    const char *find(const char *set) const {
        if (!p) return 0;
        size_t off = strcspn(p, set);
        return off >= len ? 0 : p + off; }
    const char *findstr(StringRef sub) {
        if (sub.len < 1) return p;
        const char *s = begin(), *e = end();
        while ((s = static_cast<const char *>(memchr(s, *sub.p, e-s)))) {
            if (sub.len > (size_t)(e-s)) return nullptr;
            if (!memcmp(s, sub.p, sub.len))
                return s; }
        return nullptr; }
    StringRef before(const char *s) const {
        return (size_t)(s-p) <= len ? StringRef(p, s-p) : StringRef(); }
    StringRef after(const char *s) const {
        return (size_t)(s-p) <= len ? StringRef(s, p+len-s) : StringRef(); }
    StringRef substr(size_t start, size_t length) const {
        if (len <= start) return 0;
        if (len <= length) return StringRef(p + start, len - start);
        return StringRef(p + start, length);
    }
};

template <class T> inline
    bool operator==(T a, const StringRef &b) { return b == a; }
template <class T> inline
    bool operator!=(T a, const StringRef &b) { return b != a; }
template <class T> inline
    bool operator>=(T a, const StringRef &b) { return b <= a; }
template <class T> inline
    bool operator>(T a, const StringRef &b) { return b < a; }
template <class T> inline
    bool operator<=(T a, const StringRef &b) { return b >= a; }
template <class T> inline
    bool operator<(T a, const StringRef &b) { return b > a; }

inline std::ostream &operator<<(std::ostream &os, const StringRef &a) {
    return a.len ? os.write(a.p, a.len) : os;  }
inline std::string &operator+=(std::string &s, const StringRef &a) {
    return a.len ? s.append(a.p, a.len) : s; }
inline std::string operator+(const StringRef &s, const StringRef &a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(const std::string &s, const StringRef &a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(const StringRef &s, const std::string &a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(const char *s, const StringRef &a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(const StringRef &s, const char *a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(cstring s, const StringRef &a) {
    std::string rv(s); rv += a; return rv; }
inline std::string operator+(const StringRef &s, cstring a) {
    std::string rv(s); rv += a; return rv; }

#endif /* P4C_LIB_STRINGREF_H_ */