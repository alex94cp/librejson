#include <gtest/gtest.h>
#include <rejson/parse.hpp>

#include <algorithm>
#include <iterator>

TEST(ParseTests, ParseNullWorks) {
	const auto value = rejson::parse("null");
	ASSERT_TRUE(value.is_null());
}

TEST(ParseTests, ParseTrueWorks) {
	const auto value = rejson::parse("true");
	ASSERT_EQ(value.as_bool(), true);
}

TEST(ParseTests, ParseFalseWorks) {
	const auto value = rejson::parse("false");
	ASSERT_EQ(value.as_bool(), false);
}

TEST(ParseTests, ParseIntWorks) {
	const auto value = rejson::parse("123");
	ASSERT_EQ(value.as_int(), 123);
}

TEST(ParseTests, ParseFloatWorks) {
	const auto value = rejson::parse("1.23");
	ASSERT_EQ(value.as_float(), 1.23f);
}

TEST(ParseTests, ParseStringWorks) {
	const auto value = rejson::parse("\"abc\"");
	ASSERT_EQ(value.as_string(), "abc");
}

TEST(ParseTests, ParseEscapedStringWorks) {
	const auto value = rejson::parse("\"\\n\"");
	ASSERT_EQ(value.as_string(), "\n");
}

TEST(ParseTests, ParseCodePointStringWorks) {
	const auto value = rejson::parse("\"\\u0041\"");
	ASSERT_EQ(value.as_string(), "\x41");
}

TEST(ParseTests, ParseStringThrowsIfUnescaped) {
	ASSERT_THROW({
		rejson::parse("\"\t\"");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseArrayWorks) {
	const rejson::Int expected[] = { 1, 2, 3 };
	const auto value = rejson::parse("[1,2,3]");
	const auto & array = value.as_array();
	ASSERT_TRUE(
		std::equal(array.begin(), array.end(),
			std::begin(expected), std::end(expected),
			[] (const rejson::Value & v, int e) {
				return v.as_int() == e;
			})
	);
}

TEST(ParseTests, ParseObjectWorks) {
	const auto value = rejson::parse("{ \"foo\": 123 }");
	const auto & object = value.as_object();
	const auto & foo = object.at("foo");
	ASSERT_EQ(foo.as_int(), 123);
}
