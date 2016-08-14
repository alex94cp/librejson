#include <gtest/gtest.h>
#include <rejson/path.hpp>
#include <rejson/value.hpp>

TEST(PathTests, GetFromObjectByDotWorks) {
	const rejson::Value root = rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {
			rejson::KeyValuePair { "bar", 123 }
		} }
	};
	const auto value = rejson::get(root, "foo.bar");
	EXPECT_TRUE(value != nullptr);
	ASSERT_EQ(value->as_int(), 123);
}

TEST(PathTests, GetFromObjectByBracketWorks) {
	const rejson::Value root = rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {
			rejson::KeyValuePair { "bar", 123 }
		} }
	};
	const auto value = rejson::get(root, "foo[bar]");
	EXPECT_TRUE(value != nullptr);
	ASSERT_EQ(value->as_int(), 123);
}

TEST(PathTests, GetFromObjectReturnsNullIfPropertyNotFound) {
	const rejson::Value root = rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {} }
	};
	const auto value = rejson::get(root, "foo.bar");
	ASSERT_TRUE(value == nullptr);
}

TEST(PathTests, GetFromArrayWorks) {
	const rejson::Value root = rejson::Object {
		rejson::KeyValuePair { "foo",
			rejson::Array { 123 }
		}
	};
	const auto value = rejson::get(root, "foo[0]");
	EXPECT_TRUE(value != nullptr);
	ASSERT_EQ(value->as_int(), 123);
}

TEST(PathTests, GetFromArrayReturnsNullIfOutOfBounds) {
	const rejson::Value root = rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Array {} }
	};
	const auto value = rejson::get(root, "foo[0]");
	ASSERT_TRUE(value == nullptr);
}

TEST(PathTests, GetFromTemporaryWorks) {
	const auto value = rejson::get(rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {
			rejson::KeyValuePair { "bar", 123 }
		} }
	}, "foo.bar");
	EXPECT_TRUE(value != rejson::detail::nullopt);
	ASSERT_EQ(value->as_int(), 123);
}

TEST(PathTests, GetValueOrReturnsExpectedValueIfFound) {
	const auto value = rejson::get_value_or(rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {
			rejson::KeyValuePair { "bar", 123 }
		} }
	}, "foo.bar", 456);
	ASSERT_EQ(value.as_int(), 123);
}

TEST(PathTests, GetValueOrReturnsDefaultValueIfNotFound) {
	const auto value = rejson::get_value_or(rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {} }
	}, "foo.bar", 456);
	ASSERT_EQ(value.as_int(), 456);
}

TEST(PathTests, GetValueOrFromTemporaryWorks) {
	const auto value = rejson::get_value_or(rejson::Object {
		rejson::KeyValuePair { "foo", rejson::Object {} }
	}, "foo.bar", 123);
	ASSERT_EQ(value.as_int(), 123);
}
