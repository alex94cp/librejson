#include <gtest/gtest.h>
#include <rejson/value.hpp>

TEST(ValueTests, IsNullReturnsTrueIfEmpty) {
	ASSERT_TRUE(rejson::Value().is_null());
}

TEST(ValueTests, IsNullReturnsTrueIfNull) {
	ASSERT_TRUE(rejson::Value(nullptr).is_null());
}

TEST(ValueTests, IsIntReturnsTrueIfInt) {
	ASSERT_TRUE(rejson::Value(123).is_int());
}

TEST(ValueTests, IsBoolReturnsTrueIfBool) {
	ASSERT_TRUE(rejson::Value(true).is_bool());
}

TEST(ValueTests, IsFloatReturnsTrueIfFloat) {
	ASSERT_TRUE(rejson::Value(1.23f).is_float());
}

TEST(ValueTests, IsStringReturnsTrueIfString) {
	ASSERT_TRUE(rejson::Value("abc").is_string());
}

TEST(ValueTests, IsArrayReturnsTrueIfArray) {
	const rejson::Array array { 1, 2, 3 };
	ASSERT_TRUE(rejson::Value(array).is_array());
}

TEST(ValueTests, IsObjectReturnsTrueIfObject) {
	const rejson::Object object {};
	ASSERT_TRUE(rejson::Value(object).is_object());
}

struct Foo {
	int bar;
};

namespace rejson {

template <>
struct to_json<Foo> {
	auto operator()(const Foo & foo) const {
		return Object {
			KeyValuePair { "bar", foo.bar }
		};
	}
};

}

TEST(ValueTests, ToJsonValueWorks) {
	const Foo expected { 123 };
	const rejson::Value value = expected;
	const auto & foo = value.as_object();
	const auto & bar = foo.at("bar");
	ASSERT_EQ(bar.as_int(), expected.bar);
}
