#include "communicake/json.hpp"

#include "test_harness.hpp"

#include <string>

using communicake::json::Value;
using communicake::json::parse;
using communicake::json::dump;

TEST(parses_a_nested_document) {
    const std::string text = R"({
        "vision": { "threshold": 20, "mode": "binary" },
        "actions": { "one": "code", "two": ["a", "b"] },
        "enabled": true,
        "empty": null
    })";
    std::string error;
    const Value root = parse(text, &error);
    CHECK(error.empty());
    CHECK(root.isObject());
    CHECK_EQ(root["vision"]["threshold"].asInt(), 20);
    CHECK_EQ(root["vision"]["mode"].asString(), std::string("binary"));
    CHECK_EQ(root["actions"]["two"].size(), std::size_t(2));
    CHECK_EQ(root["actions"]["two"][1].asString(), std::string("b"));
    CHECK(root["enabled"].asBool());
    CHECK(root["empty"].isNull());
}

TEST(missing_keys_yield_null) {
    const Value root = parse(R"({"a": 1})");
    CHECK(root["b"].isNull());
    CHECK(root["a"]["b"].isNull());
    CHECK_EQ(root["b"].asInt(7), 7);
    CHECK_EQ(root["b"].asString("fallback"), std::string("fallback"));
}

TEST(numbers_are_parsed) {
    CHECK_NEAR(parse("1").asNumber(), 1.0, 1e-9);
    CHECK_NEAR(parse("-1.5").asNumber(), -1.5, 1e-9);
    CHECK_NEAR(parse("2.5e2").asNumber(), 250.0, 1e-9);
    CHECK_NEAR(parse("1E-2").asNumber(), 0.01, 1e-9);
}

TEST(string_escapes_are_decoded) {
    const Value value = parse(R"("line\nbreak\t\"quoted\"\\slash\/")");
    CHECK_EQ(value.asString(), std::string("line\nbreak\t\"quoted\"\\slash/"));
}

TEST(unicode_escapes_are_decoded_to_utf8) {
    const Value value = parse(R"("caf\u00e9")");
    CHECK_EQ(value.asString(), std::string("caf\xC3\xA9"));

    const Value emoji = parse(R"("\uD83D\uDE00")");
    CHECK_EQ(emoji.asString().size(), std::size_t(4));
}

TEST(comments_and_trailing_commas_are_rejected) {
    std::string error;
    parse("{\"a\": 1,}", &error);
    CHECK(!error.empty());
    CHECK(error.find("line") == 0);
}

TEST(malformed_input_reports_a_position) {
    std::string error;
    parse("{\n  \"a\" 1\n}", &error);
    CHECK(error.find("line 2") == 0);
    CHECK(error.find("column") != std::string::npos);

    error.clear();
    parse("\"unterminated", &error);
    CHECK(error.find("unterminated") != std::string::npos);

    error.clear();
    parse("{} trailing", &error);
    CHECK(error.find("trailing") != std::string::npos);

    error.clear();
    parse("", &error);
    CHECK(!error.empty());
}

TEST(dump_round_trips) {
    const std::string text = R"({"a":1,"b":["x",true,null],"c":{"d":-2.5}})";
    const Value original = parse(text);
    const Value restored = parse(dump(original));
    CHECK_EQ(dump(restored), dump(original));
    CHECK_EQ(restored["b"][1].asBool(), true);
    CHECK_NEAR(restored["c"]["d"].asNumber(), -2.5, 1e-9);
}

TEST(dump_escapes_strings) {
    Value value;
    value.set("key", Value("quote \" newline \n tab \t"));
    const std::string text = dump(value);
    const Value restored = parse(text);
    CHECK_EQ(restored["key"].asString(), std::string("quote \" newline \n tab \t"));
}

TEST(dump_can_emit_a_single_line) {
    Value value;
    value.set("a", Value(1));
    const std::string text = dump(value, 0);
    CHECK(text.find('\n') == std::string::npos);
}

TEST_MAIN
