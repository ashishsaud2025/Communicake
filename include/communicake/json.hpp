#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// A deliberately small JSON implementation: just enough for configuration
// files, with no third-party dependency. Objects, arrays, strings with escapes,
// numbers, booleans and null are supported.
namespace communicake::json {

enum class Type { Null, Bool, Number, String, Array, Object };

class Value {
  public:
    Value() = default;
    explicit Value(bool value);
    explicit Value(double value);
    explicit Value(int value);
    explicit Value(const char* value);
    explicit Value(std::string value);

    Type type() const;
    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    int asInt(int fallback = 0) const;
    std::string asString(const std::string& fallback = "") const;

    bool has(const std::string& key) const;
    const std::vector<Value>& items() const;
    const std::map<std::string, Value>& members() const;
    std::size_t size() const;

    /// Missing keys and out-of-range indices yield a null value rather than
    /// throwing, which keeps configuration reading terse.
    const Value& operator[](const std::string& key) const;
    const Value& operator[](std::size_t index) const;

    /// Insert or overwrite a member. The parser refuses duplicate keys before
    /// reaching this, so an overwrite only happens when a caller asks for it.
    void set(const std::string& key, Value value);
    void push(Value value);

    static const Value& nullValue();

  private:
    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    std::vector<Value> array_;
    std::map<std::string, Value> object_;
};

/// Parse `text`. On failure returns a null value and, when `error` is given,
/// writes a message of the form "line 3, column 5: ...".
Value parse(const std::string& text, std::string* error = nullptr);

/// Serialise with `indent` spaces per level; `indent` of 0 emits one line.
std::string dump(const Value& value, int indent = 2);

} // namespace communicake::json
