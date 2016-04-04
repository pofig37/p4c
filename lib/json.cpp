#include <stdexcept>
#include <sstream>
#include "json.h"
#include "indent.h"

namespace Util {

cstring IJson::toString() const {
    std::stringstream str;
    serialize(str);
    return cstring(str.str());
}

JsonValue* JsonValue::null = new JsonValue();

void JsonValue::serialize(std::ostream& out) const {
    switch (tag) {
        case Kind::String:
            out << "\"" << str << "\"";
            break;
        case Kind::Number:
            out << value;
            break;
        case Kind::True:
            out << "true";
            break;
        case Kind::False:
            out << "false";
            break;
        case Kind::Null:
            out << "null";
            break;
    }
}

bool JsonValue::operator==(const bool& b) const
{ return b ? tag == Kind::True : tag == Kind::False; }
bool JsonValue::operator==(const mpz_class& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const int& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const long& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const unsigned& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const unsigned long& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const double& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const float& v) const
{ return tag == Kind::Number ? v == value : false; }
bool JsonValue::operator==(const cstring& s) const
{ return tag == Kind::String ? s == str : false; }
bool JsonValue::operator==(const std::string& s) const
{ return tag == Kind::String ? cstring(s) == str : false; }
bool JsonValue::operator==(const char* s) const
{ return tag == Kind::String ? cstring(s) == str : false; }
bool JsonValue::operator==(const JsonValue& other) const {
    if (tag != other.tag) return false;
    switch (tag) {
        case Kind::String:
            return str == other.str;
        case Kind::Number:
            return value == other.value;
        case Kind::True:
        case Kind::False:
        case Kind::Null:
            return true;
    }
}

void JsonArray::serialize(std::ostream& out) const {
    bool isSmall = true;
    for (auto v : values) {
        if (!v->is<JsonValue>())
            isSmall = false;
    }
    out << "[";
    if (!isSmall)
        out << IndentCtl::indent;
    bool first = true;
    for (auto v : values) {
        if (!first) {
            out << ",";
            if (isSmall)
                out << " ";
        }
        if (!isSmall)
            out << IndentCtl::endl;
        if (v == nullptr)
            out << "null";
        else
            v->serialize(out);
        first = false;
    }
    if (!isSmall)
        out << IndentCtl::unindent << IndentCtl::endl;
    out << "]";
}

bool JsonValue::getBool() const {
    if (!isBool())
        throw std::logic_error("Incorrect json value kind");
    return tag == Kind::True;
}

cstring JsonValue::getString() const {
    if (!isString())
        throw std::logic_error("Incorrect json value kind");
    return str;
}

mpz_class JsonValue::getValue() const {
    if (!isNumber())
        throw std::logic_error("Incorrect json value kind");
    return value;
}

JsonArray* JsonArray::append(const IJson* value) {
    values.push_back(value);
    return this;
}

void JsonObject::serialize(std::ostream& out) const {
    out << "{" << IndentCtl::indent;
    bool first = true;
    for (auto it : labelOrder) {
        if (!first)
            out << ",";
        first = false;
        out << IndentCtl::endl;
        out << "\"" << it << "\"" << " : ";
        auto j = get(it);
        if (j == nullptr)
            out << "null";
        else
            j->serialize(out);
    }
    out << IndentCtl::unindent << IndentCtl::endl << "}";
}

JsonObject* JsonObject::emplace(cstring label, const IJson* value) {
    if (label.isNullOrEmpty())
        throw std::logic_error("Empty label");
    auto j = get(label);
    if (j != nullptr)
        throw std::logic_error(cstring("Duplicate label in json object ") + label.c_str());
    values.emplace(label, value);
    labelOrder.push_back(label);
    return this;
}

}  // namespace Util