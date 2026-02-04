// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Minimal JSON parser for the miner tool. No external dependencies.
// Supports: objects, arrays, strings, numbers, booleans, null.

#ifndef DOGED_TOOLS_MINER_JSON_H
#define DOGED_TOOLS_MINER_JSON_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace json {

enum class Type { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT };

class Value {
public:
    Value() : m_type(Type::NUL) {}
    Value(bool b) : m_type(Type::BOOL), m_bool(b) {}
    Value(double n) : m_type(Type::NUMBER), m_num(n) {}
    Value(const std::string &s) : m_type(Type::STRING), m_str(s) {}
    Value(const char *s) : m_type(Type::STRING), m_str(s) {}

    static Value array() {
        Value v;
        v.m_type = Type::ARRAY;
        return v;
    }
    static Value object() {
        Value v;
        v.m_type = Type::OBJECT;
        return v;
    }

    Type type() const { return m_type; }
    bool is_null() const { return m_type == Type::NUL; }
    bool is_bool() const { return m_type == Type::BOOL; }
    bool is_number() const { return m_type == Type::NUMBER; }
    bool is_string() const { return m_type == Type::STRING; }
    bool is_array() const { return m_type == Type::ARRAY; }
    bool is_object() const { return m_type == Type::OBJECT; }

    bool get_bool() const { return m_bool; }
    double get_number() const { return m_num; }
    const std::string &get_string() const { return m_str; }

    size_t size() const {
        if (m_type == Type::ARRAY) return m_arr.size();
        if (m_type == Type::OBJECT) return m_obj.size();
        return 0;
    }

    void push_back(const Value &v) { m_arr.push_back(v); }

    const Value &operator[](size_t i) const {
        static Value null_val;
        return i < m_arr.size() ? m_arr[i] : null_val;
    }
    const Value &operator[](const std::string &key) const {
        static Value null_val;
        auto it = m_obj.find(key);
        return it != m_obj.end() ? it->second : null_val;
    }

    bool contains(const std::string &key) const {
        return m_obj.find(key) != m_obj.end();
    }

    void set(const std::string &key, const Value &v) { m_obj[key] = v; }

private:
    Type m_type = Type::NUL;
    bool m_bool = false;
    double m_num = 0;
    std::string m_str;
    std::vector<Value> m_arr;
    std::map<std::string, Value> m_obj;
};

namespace detail {

inline void skip_ws(const std::string &s, size_t &i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' ||
                            s[i] == '\n'))
        ++i;
}

inline Value parse_value(const std::string &s, size_t &i);

inline std::string parse_string(const std::string &s, size_t &i) {
    if (s[i] != '"') throw std::runtime_error("expected '\"'");
    ++i;
    std::string result;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\') {
            ++i;
            if (i >= s.size()) break;
            switch (s[i]) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += s[i]; break;
            }
        } else {
            result += s[i];
        }
        ++i;
    }
    if (i < s.size()) ++i; // skip closing '"'
    return result;
}

inline Value parse_number(const std::string &s, size_t &i) {
    size_t start = i;
    if (s[i] == '-') ++i;
    while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) ++i;
    if (i < s.size() && s[i] == '.') {
        ++i;
        while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) ++i;
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
        while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) ++i;
    }
    return Value(std::stod(s.substr(start, i - start)));
}

inline Value parse_array(const std::string &s, size_t &i) {
    Value arr = Value::array();
    ++i; // skip '['
    skip_ws(s, i);
    if (i < s.size() && s[i] == ']') { ++i; return arr; }
    while (i < s.size()) {
        arr.push_back(parse_value(s, i));
        skip_ws(s, i);
        if (i < s.size() && s[i] == ',') { ++i; skip_ws(s, i); continue; }
        if (i < s.size() && s[i] == ']') { ++i; break; }
        break;
    }
    return arr;
}

inline Value parse_object(const std::string &s, size_t &i) {
    Value obj = Value::object();
    ++i; // skip '{'
    skip_ws(s, i);
    if (i < s.size() && s[i] == '}') { ++i; return obj; }
    while (i < s.size()) {
        skip_ws(s, i);
        std::string key = parse_string(s, i);
        skip_ws(s, i);
        if (i < s.size() && s[i] == ':') ++i;
        skip_ws(s, i);
        obj.set(key, parse_value(s, i));
        skip_ws(s, i);
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == '}') { ++i; break; }
        break;
    }
    return obj;
}

inline Value parse_value(const std::string &s, size_t &i) {
    skip_ws(s, i);
    if (i >= s.size()) return Value();
    char c = s[i];
    if (c == '"') return Value(parse_string(s, i));
    if (c == '{') return parse_object(s, i);
    if (c == '[') return parse_array(s, i);
    if (c == 't') { i += 4; return Value(true); }
    if (c == 'f') { i += 5; return Value(false); }
    if (c == 'n') { i += 4; return Value(); }
    return parse_number(s, i);
}

} // namespace detail

inline Value parse(const std::string &s) {
    size_t i = 0;
    return detail::parse_value(s, i);
}

} // namespace json

#endif
