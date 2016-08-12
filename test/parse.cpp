#include <gtest/gtest.h>
#include <rejson/parse.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>

TEST(ParseTests, ParseNullWorks) {
	const auto value = rejson::parse("null");
	ASSERT_TRUE(value.is_null());
}

TEST(ParseTests, ParseUndefinedThrows) {
	ASSERT_THROW({
		rejson::parse("undefined");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseTrueWorks) {
	const auto value = rejson::parse("true");
	ASSERT_EQ(value.as_bool(), true);
}

TEST(ParseTests, ParseFalseWorks) {
	const auto value = rejson::parse("false");
	ASSERT_EQ(value.as_bool(), false);
}

TEST(ParseTests, ParseInvalidTrueThrows) {
	ASSERT_THROW({
		rejson::parse("True");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseInvalidFalseThrows) {
	ASSERT_THROW({
		rejson::parse("False");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseNumberGivesInt) {
	const auto value = rejson::parse("123");
	ASSERT_EQ(value.as_int(), 123);
}

TEST(ParseTests, ParseDecimalNumberGivesReal) {
	const auto value = rejson::parse("1.23");
	ASSERT_EQ(value.as_real(), 1.23);
}

TEST(ParseTests, ParseNumberWithExpGivesReal) {
	const auto value = rejson::parse("12e3");
	ASSERT_EQ(value.as_real(), 12e3);
}

TEST(ParseTests, ParsePositiveSignedNumberThrows) {
	ASSERT_THROW({
		rejson::parse("+123");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseNegativeSignedNumberGivesInt) {
	const auto value = rejson::parse("-123");
	ASSERT_EQ(value.as_int(), -123);
}

TEST(ParseTests, ParseNegativeSignedDecimalNumberWorks) {
	const auto value = rejson::parse("-1.23");
	ASSERT_EQ(value.as_real(), -1.23);
}

TEST(ParseTests, ParseNumberWithPositiveSignedExpWorks) {
	const auto value = rejson::parse("12e+3");
	ASSERT_EQ(value.as_real(), 12e3);
}

TEST(ParseTests, ParseNumberWithNegativeSignedExpWorks) {
	const auto value = rejson::parse("12e-3");
	ASSERT_EQ(value.as_real(), 12e-3);
}

TEST(ParseTests, ParseStringWorks) {
	const auto value = rejson::parse("\"abc\"");
	ASSERT_EQ(value.as_string(), "abc");
}

TEST(ParseTests, ParseEmptyStringWorks) {
	const auto value = rejson::parse("\"\"");
	ASSERT_EQ(value.as_string(), "");
}

TEST(ParseTests, ParseSingleQuotedStringThrows) {
	ASSERT_THROW({
		rejson::parse("'abc'");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseStringWithMissingEndingQuoteThrows) {
	ASSERT_THROW({
		rejson::parse("\"abc");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseEscapedStringWorks) {
	const auto value = rejson::parse("\"\\n\"");
	ASSERT_EQ(value.as_string(), "\n");
}

TEST(ParseTests, ParseCodePointStringWorks) {
	const auto value = rejson::parse("\"\\u0041\"");
	ASSERT_EQ(value.as_string(), "\x41");
}

TEST(ParseTests, ParseUnescapedStringThrows) {
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

TEST(ParseTests, ParseEmptyArrayWorks) {
	const auto value = rejson::parse("[]");
	ASSERT_EQ(value.as_array().size(), 0);
}

TEST(ParseTests, ParseArrayWithTrailingCommaThrows) {
	ASSERT_THROW({
		rejson::parse("[1,]");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseArrayWithConsecutiveCommasThrows) {
	ASSERT_THROW({
		rejson::parse("[1,,2]");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseArrayWithUndefinedItemThrows) {
	ASSERT_THROW({
		rejson::parse("[undefined]");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWorks) {
	const auto value = rejson::parse("{ \"foo\": 123 }");
	const auto & object = value.as_object();
	const auto & foo = object.at("foo");
	ASSERT_EQ(foo.as_int(), 123);
}

TEST(ParseTests, ParseEmptyObjectWorks) {
	const auto value = rejson::parse("{}");
	ASSERT_EQ(value.as_object().size(), 0);
}

TEST(ParseTests, ParseObjectWithNullKeyThrows) {
	ASSERT_THROW({
		rejson::parse("{ null: 123 }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithBoolKeyThrows) {
	ASSERT_THROW({
		rejson::parse("{ true: 123 }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithTrailingCommaThrows) {
	ASSERT_THROW({
		rejson::parse("{ \"foo\": 123, }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithNumericKeyThrows) {
	ASSERT_THROW({
		rejson::parse("{ 123: 456 }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithSingleQuotedKeyThrows) {
	ASSERT_THROW({
		rejson::parse("{ 'foo': 123 }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithUnquotedKeyThrows) {
	ASSERT_THROW({
		rejson::parse("{ foo: 123 }");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithSingleLineCommentThrows) {
	ASSERT_THROW({
		rejson::parse(R"({
			"foo": 123 // comment
		})");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithMultilineCommentThrows) {
	ASSERT_THROW({
		rejson::parse(R"({
			"foo": 123 /* comment */
		})");
	}, rejson::ParseError);
}

TEST(ParseTests, ParseObjectWithValueAloneThrows) {
	ASSERT_THROW({
		rejson::parse("{ 123 }");
	}, rejson::ParseError);
}
