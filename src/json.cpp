#include "communicake/json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace communicake::json {
namespace {

const Value& nullSingleton() {
    static const Value value;
    return value;
}

class Parser {
  public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool parseDocument(Value& out, std::string& error) {
        skipWhitespace();
        if (!parseValue(out, error)) {
            return false;
        }
        skipWhitespace();
        if (!atEnd()) {
            return fail(error, "unexpected trailing content");
        }
        return true;
    }

  private:
    bool atEnd() const {
        return position_ >= text_.size();
    }

    char peek() const {
        return atEnd() ? '\0' : text_[position_];
    }

    char take() {
        if (atEnd()) {
            return '\0';
        }
        const char c = text_[position_++];
        if (c == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        return c;
    }

    void skipWhitespace() {
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                take();
            } else {
                break;
            }
        }
    }

    bool fail(std::string& error, const std::string& message) const {
        error = "line " + std::to_string(line_) + ", column " + std::to_string(column_) + ": " +
                message;
        return false;
    }

    bool parseValue(Value& out, std::string& error) {
        skipWhitespace();
        if (atEnd()) {
            return fail(error, "unexpected end of input");
        }
        const char c = peek();
        if (c == '{') {
            return parseObject(out, error);
        }
        if (c == '[') {
            return parseArray(out, error);
        }
        if (c == '"') {
            std::string text;
            if (!parseString(text, error)) {
                return false;
            }
            out = Value(std::move(text));
            return true;
        }
        if (c == 't') {
            return parseLiteral("true", Value(true), out, error);
        }
        if (c == 'f') {
            return parseLiteral("false", Value(false), out, error);
        }
        if (c == 'n') {
            return parseLiteral("null", Value(), out, error);
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(out, error);
        }
        return fail(error, std::string("unexpected character '") + c + "'");
    }

    bool parseLiteral(const char* literal, Value value, Value& out, std::string& error) {
        for (const char* p = literal; *p != '\0'; ++p) {
            if (peek() != *p) {
                return fail(error, "invalid literal, expected " + std::string(literal));
            }
            take();
        }
        out = std::move(value);
        return true;
    }

    bool parseObject(Value& out, std::string& error) {
        take(); // '{'
        Value object;
        skipWhitespace();
        if (peek() == '}') {
            take();
            out = std::move(object);
            return true;
        }
        while (true) {
            skipWhitespace();
            if (peek() != '"') {
                return fail(error, "expected a quoted key");
            }
            std::string key;
            if (!parseString(key, error)) {
                return false;
            }
            skipWhitespace();
            if (peek() != ':') {
                return fail(error, "expected ':' after key \"" + key + "\"");
            }
            take();
            Value member;
            if (!parseValue(member, error)) {
                return false;
            }
            // A duplicate key is almost always a copy-and-paste mistake, and
            // silently keeping the last one hides it. Every mainstream JSON
            // parser this could replace rejects it, so do the same.
            if (object.has(key)) {
                return fail(error, "duplicate key \"" + key + "\"");
            }
            object.set(key, std::move(member));
            skipWhitespace();
            const char next = peek();
            if (next == ',') {
                take();
                skipWhitespace();
                if (peek() == '}') {
                    return fail(error, "trailing comma in object");
                }
                continue;
            }
            if (next == '}') {
                take();
                out = std::move(object);
                return true;
            }
            return fail(error, "expected ',' or '}' in object");
        }
    }

    bool parseArray(Value& out, std::string& error) {
        take(); // '['
        Value array;
        skipWhitespace();
        if (peek() == ']') {
            take();
            out = std::move(array);
            return true;
        }
        while (true) {
            Value item;
            if (!parseValue(item, error)) {
                return false;
            }
            array.push(std::move(item));
            skipWhitespace();
            const char next = peek();
            if (next == ',') {
                take();
                skipWhitespace();
                if (peek() == ']') {
                    return fail(error, "trailing comma in array");
                }
                continue;
            }
            if (next == ']') {
                take();
                out = std::move(array);
                return true;
            }
            return fail(error, "expected ',' or ']' in array");
        }
    }

    static void appendUtf8(std::string& out, unsigned codepoint) {
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    bool parseHex4(unsigned& value, std::string& error) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = take();
            int digit = -1;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10;
            }
            if (digit < 0) {
                return fail(error, "invalid \\u escape");
            }
            value = value * 16 + static_cast<unsigned>(digit);
        }
        return true;
    }

    bool parseString(std::string& out, std::string& error) {
        take(); // opening quote
        out.clear();
        while (true) {
            if (atEnd()) {
                return fail(error, "unterminated string");
            }
            const char c = take();
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            const char escape = take();
            switch (escape) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u': {
                unsigned codepoint = 0;
                if (!parseHex4(codepoint, error)) {
                    return false;
                }
                if (codepoint >= 0xD800 && codepoint <= 0xDBFF && peek() == '\\') {
                    const std::size_t savedPosition = position_;
                    const int savedLine = line_;
                    const int savedColumn = column_;
                    take();
                    if (take() == 'u') {
                        unsigned low = 0;
                        if (!parseHex4(low, error)) {
                            return false;
                        }
                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                        } else {
                            return fail(error, "invalid surrogate pair");
                        }
                    } else {
                        position_ = savedPosition;
                        line_ = savedLine;
                        column_ = savedColumn;
                    }
                }
                appendUtf8(out, codepoint);
                break;
            }
            default:
                return fail(error, std::string("invalid escape '\\") + escape + "'");
            }
        }
    }

    bool parseNumber(Value& out, std::string& error) {
        const std::size_t start = position_;
        if (peek() == '-') {
            take();
        }
        while (!atEnd() && peek() >= '0' && peek() <= '9') {
            take();
        }
        if (peek() == '.') {
            take();
            if (!(peek() >= '0' && peek() <= '9')) {
                return fail(error, "expected digits after the decimal point");
            }
            while (!atEnd() && peek() >= '0' && peek() <= '9') {
                take();
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            take();
            if (peek() == '+' || peek() == '-') {
                take();
            }
            if (!(peek() >= '0' && peek() <= '9')) {
                return fail(error, "expected digits in the exponent");
            }
            while (!atEnd() && peek() >= '0' && peek() <= '9') {
                take();
            }
        }
        const std::string numberText = text_.substr(start, position_ - start);
        char* end = nullptr;
        const double value = std::strtod(numberText.c_str(), &end);
        if (end == numberText.c_str() || *end != '\0' || !std::isfinite(value)) {
            return fail(error, "invalid number");
        }
        out = Value(value);
        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
    int line_ = 1;
    int column_ = 1;
};

void dumpString(const std::string& text, std::string& out) {
    out.push_back('"');
    for (const char c : text) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(c));
                out += buffer;
            } else {
                out.push_back(c);
            }
        }
    }
    out.push_back('"');
}

void dumpValue(const Value& value, int indent, int depth, std::string& out) {
    const std::string pad = indent > 0 ? std::string(static_cast<std::size_t>(indent * depth), ' ') : "";
    const std::string childPad =
        indent > 0 ? std::string(static_cast<std::size_t>(indent * (depth + 1)), ' ') : "";
    const std::string newline = indent > 0 ? "\n" : "";
    const std::string space = indent > 0 ? " " : "";

    switch (value.type()) {
    case Type::Null:
        out += "null";
        break;
    case Type::Bool:
        out += value.asBool() ? "true" : "false";
        break;
    case Type::Number: {
        std::ostringstream stream;
        stream << value.asNumber();
        out += stream.str();
        break;
    }
    case Type::String:
        dumpString(value.asString(), out);
        break;
    case Type::Array: {
        if (value.items().empty()) {
            out += "[]";
            break;
        }
        out += "[" + newline;
        for (std::size_t i = 0; i < value.items().size(); ++i) {
            out += childPad;
            dumpValue(value.items()[i], indent, depth + 1, out);
            if (i + 1 < value.items().size()) {
                out += ",";
            }
            out += newline;
        }
        out += pad + "]";
        break;
    }
    case Type::Object: {
        if (value.members().empty()) {
            out += "{}";
            break;
        }
        out += "{" + newline;
        std::size_t index = 0;
        for (const auto& entry : value.members()) {
            out += childPad;
            dumpString(entry.first, out);
            out += ":" + space;
            dumpValue(entry.second, indent, depth + 1, out);
            if (++index < value.members().size()) {
                out += ",";
            }
            out += newline;
        }
        out += pad + "}";
        break;
    }
    }
}

} // namespace

Value::Value(bool value) : type_(Type::Bool), bool_(value) {}
Value::Value(double value) : type_(Type::Number), number_(value) {}
Value::Value(int value) : type_(Type::Number), number_(static_cast<double>(value)) {}
Value::Value(const char* value) : type_(Type::String), string_(value == nullptr ? "" : value) {}
Value::Value(std::string value) : type_(Type::String), string_(std::move(value)) {}

Type Value::type() const {
    return type_;
}

bool Value::isNull() const {
    return type_ == Type::Null;
}
bool Value::isBool() const {
    return type_ == Type::Bool;
}
bool Value::isNumber() const {
    return type_ == Type::Number;
}
bool Value::isString() const {
    return type_ == Type::String;
}
bool Value::isArray() const {
    return type_ == Type::Array;
}
bool Value::isObject() const {
    return type_ == Type::Object;
}

bool Value::asBool(bool fallback) const {
    return type_ == Type::Bool ? bool_ : fallback;
}

double Value::asNumber(double fallback) const {
    return type_ == Type::Number ? number_ : fallback;
}

int Value::asInt(int fallback) const {
    return type_ == Type::Number ? static_cast<int>(std::lround(number_)) : fallback;
}

std::string Value::asString(const std::string& fallback) const {
    return type_ == Type::String ? string_ : fallback;
}

bool Value::has(const std::string& key) const {
    return type_ == Type::Object && object_.find(key) != object_.end();
}

const std::vector<Value>& Value::items() const {
    return array_;
}

const std::map<std::string, Value>& Value::members() const {
    return object_;
}

std::size_t Value::size() const {
    if (type_ == Type::Array) {
        return array_.size();
    }
    if (type_ == Type::Object) {
        return object_.size();
    }
    return 0;
}

const Value& Value::operator[](const std::string& key) const {
    if (type_ != Type::Object) {
        return nullSingleton();
    }
    const auto it = object_.find(key);
    return it == object_.end() ? nullSingleton() : it->second;
}

const Value& Value::operator[](std::size_t index) const {
    if (type_ != Type::Array || index >= array_.size()) {
        return nullSingleton();
    }
    return array_[index];
}

void Value::set(const std::string& key, Value value) {
    if (type_ != Type::Object) {
        type_ = Type::Object;
        array_.clear();
    }
    object_[key] = std::move(value);
}

void Value::push(Value value) {
    if (type_ != Type::Array) {
        type_ = Type::Array;
        object_.clear();
    }
    array_.push_back(std::move(value));
}

const Value& Value::nullValue() {
    return nullSingleton();
}

Value parse(const std::string& text, std::string* error) {
    Parser parser(text);
    Value value;
    std::string message;
    if (!parser.parseDocument(value, message)) {
        if (error != nullptr) {
            *error = message;
        }
        return Value();
    }
    if (error != nullptr) {
        error->clear();
    }
    return value;
}

std::string dump(const Value& value, int indent) {
    std::string out;
    dumpValue(value, indent < 0 ? 0 : indent, 0, out);
    return out;
}

} // namespace communicake::json
