#include <rejson/value.hpp>

#include <boost/variant/get.hpp>

namespace rejson {

Value::Value() noexcept {}
Value::Value(const Value & other) = default;
Value::Value(Value && other) = default;

Value::Value(Null n) noexcept
	: value_ {} {}

Value::Value(Int i)
	: value_ { i } {}

Value::Value(Real r)
	: value_ { r } {}

Value::Value(Bool b)
	: value_ { b } {}

Value::Value(String s)
	: value_ { std::move(s) } {}

Value::Value(Array a)
	: value_ { std::move(a) } {}

Value::Value(Object o)
	: value_ { std::move(o) } {}

Value::Value(const char * s)
	: Value { std::string(s) } {}

ValueType Value::type() const
{
	return static_cast<ValueType>(value_.which());
}

bool Value::is_int() const
{
	return type() == ValueType::Int;
}

bool Value::is_null() const
{
	return type() == ValueType::Null;
}

bool Value::is_real() const
{
	return type() == ValueType::Real;
}

bool Value::is_bool() const
{
	return type() == ValueType::Bool;
}

bool Value::is_string() const
{
	return type() == ValueType::String;
}

bool Value::is_array() const
{
	return type() == ValueType::Array;
}

bool Value::is_object() const
{
	return type() == ValueType::Object;
}

Int Value::as_int() const
{
	return boost::get<Int>(value_);
}

Bool Value::as_bool() const
{
	return boost::get<Bool>(value_);
}

Real Value::as_real() const
{
	return boost::get<Real>(value_);
}

Array Value::as_array() &&
{
	return std::move(boost::get<Array>(value_));
}

Array & Value::as_array() &
{
	return boost::get<Array>(value_);
}

const Array & Value::as_array() const &
{
	return boost::get<Array>(value_);
}

String Value::as_string() &&
{
	return std::move(boost::get<String>(value_));
}

String & Value::as_string() &
{
	return boost::get<String>(value_);
}

const String & Value::as_string() const &
{
	return boost::get<String>(value_);
}

Object Value::as_object() &&
{
	return std::move(boost::get<Object>(value_));
}

Object & Value::as_object() &
{
	return boost::get<Object>(value_);
}

const Object & Value::as_object() const &
{
	return boost::get<Object>(value_);
}

void Value::swap(Value & other)
{
	value_.swap(other.value_);
}

Value & Value::operator=(Array && a)
{
	value_ = std::move(a);
	return *this;
}

Value & Value::operator=(String && s)
{
	value_ = std::move(s);
	return *this;
}

Value & Value::operator=(Object && o)
{
	value_ = std::move(o);
	return *this;
}

Value & Value::operator=(const Value & other) = default;
Value & Value::operator=(Value && other) = default;

}
