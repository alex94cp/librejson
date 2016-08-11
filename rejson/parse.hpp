#ifndef REJSON_PARSE_HPP_
#define REJSON_PARSE_HPP_

#include <rejson/value.hpp>

#include <boost/spirit/home/support/iterators/istream_iterator.hpp>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <istream>
#include <sstream>
#include <stdexcept>

#ifdef __has_include
#	if __has_include(<string_view>)
#		include <string_view>
#		define rejson_have_string_view 1
#	elif __has_include(<experimental/string_view>)
#		include <experimental/string_view>
#		define rejson_have_string_view 1
#		define rejson_experimental_string_view 1
#	else
#		define rejson_have_string_view 0
#	endif
#endif

#if !rejson_have_string_view
#	include <boost/utility/string_ref.hpp>
#endif

namespace rejson {

#if rejson_have_string_view
#	ifdef rejson_experimental_string_view
		using std::experimental::basic_string_view;
#	else
		using std::basic_string_view;
#	endif
#else
	template <typename T>
	using basic_string_view = boost::basic_string_ref<T>;
#endif

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

class REJSON_EXPORT ParseError : public std::runtime_error
{
	using runtime_error::runtime_error;
};

REJSON_EXPORT Value parse(string_view sv);
REJSON_EXPORT Value parse(wstring_view sv);
REJSON_EXPORT Value parse(u16string_view sv);
REJSON_EXPORT Value parse(u32string_view sv);

template <class Iterator>
Value parse(Iterator begin, Iterator end);

template <typename CharT>
Value parse(std::basic_istream<CharT> & is);

namespace detail {

template <class Iterator>
Value parse_value(Iterator begin, Iterator end, Iterator & iter);

void encode_utf8(char32_t code_pt, std::ostringstream & os);

template <class Iterator, typename CharT>
void consume(Iterator begin, Iterator end,
             CharT token, Iterator & iter)
{
	using namespace std::literals::string_literals;

	iter = begin;
	if (iter == end)
		throw ParseError("unexpected end of input");
	if (*iter != token)
		throw ParseError("expected '"s + token + "' token");
	++iter;
}

template <class Iterator, typename CharT>
bool try_consume(Iterator begin, Iterator end,
                 CharT token, Iterator & iter)
{
	iter = begin;
	if (iter == end || *iter != token)
		return false;
	return ++iter, true;
}

template <class Iterator, typename CharT>
bool try_consume(Iterator begin, Iterator end,
                 const CharT * tokens, Iterator & iter)
{
	iter = begin;
	auto itercmp = iter;
	while (*tokens) {
		if (itercmp == end)
			return false;
		if (*itercmp++ != *tokens++)
			return false;
	}
	iter = itercmp;
	return true;
}

template <class Iterator>
Null parse_null(Iterator begin, Iterator end, Iterator & iter)
{
	if (!try_consume(begin, end, "null", iter))
		throw ParseError("invalid value");
	return nullptr;
}

template <class Iterator>
Bool parse_true(Iterator begin, Iterator end, Iterator & iter)
{
	if (!try_consume(begin, end, "true", iter))
		throw ParseError("invalid value");
	return true;
}

template <class Iterator>
Bool parse_false(Iterator begin, Iterator end, Iterator & iter)
{
	if (!try_consume(begin, end, "false", iter))
		throw ParseError("invalid value");
	return false;
}

template <class Iterator>
bool try_parse_xdigit(Iterator begin, Iterator end,
                      Iterator & iter, std::uint8_t & xdigit)
{
	iter = begin;
	if (iter == end)
		return false;
	const auto c = *iter;
	if (std::isdigit(c)) {
		xdigit = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		xdigit = 10 + (c - 'a');
	} else if (c >= 'A' && c <= 'F') {
		xdigit = 10 + (c - 'A');
	} else {
		return false;
	}
	return ++iter, true;
}

template <class Iterator>
bool try_parse_codept(Iterator begin, Iterator end,
                      Iterator & iter, char16_t & code_pt)
{
	Iterator try_iter;
	std::uint8_t d0, d1, d2, d3;
	if (!try_consume(begin, end, "\\u", try_iter)
	    || !try_parse_xdigit(try_iter, end, try_iter, d0)
	    || !try_parse_xdigit(try_iter, end, try_iter, d1)
	    || !try_parse_xdigit(try_iter, end, try_iter, d2)
	    || !try_parse_xdigit(try_iter, end, try_iter, d3))
		return false;
	code_pt = d0 << ((CHAR_BIT / 2) * 3)
	        | d1 << ((CHAR_BIT / 2) * 2)
	        | d2 << ((CHAR_BIT / 2) * 1)
	        | d3 << ((CHAR_BIT / 2) * 0);
	iter = try_iter;
	return true;
}

template <typename T, typename Min, typename Max>
constexpr bool in_range(T x, Min min, Max max)
{
	return x >= min && x <= max;
}

template <class Iterator, typename CharT = decltype(*std::declval<Iterator>())>
String parse_string(Iterator begin, Iterator end, Iterator & iter)
{
	std::ostringstream oss;
	consume(begin, end, '"', iter);
	long last_code_pt = -1;
	while (iter != end) {
		switch (*iter) {
		case '"':
			if (last_code_pt != -1)
				encode_utf8(last_code_pt, oss);
			return ++iter, oss.str();
		case '\\': {
			char16_t code_pt;
			if (try_parse_codept(iter, end, iter, code_pt)) {
				if (in_range(last_code_pt, 0xd800, 0xdbff)
				    && in_range(code_pt, 0xdc00, 0xdfff)) {
					const auto hi = last_code_pt - 0xd800;
					const auto lo = code_pt - 0xdc00 + 0x10000;
					encode_utf8(hi << 10 | lo, oss);
					last_code_pt = -1;
				} else {
					if (last_code_pt != -1)
						encode_utf8(code_pt, oss);
					last_code_pt = code_pt;
				}
				break;
			} else {
				++iter;
			} }
		default:
			if (last_code_pt != -1)
				encode_utf8(last_code_pt, oss);
			last_code_pt = -1;
			oss.put(*iter++);
		}
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator>
void skip_whitespace(Iterator begin, Iterator end, Iterator & iter)
{
	for (iter = begin; iter != end; ++iter) {
		if (!std::isspace(*iter))
			return;
	}
}

template <class Iterator>
Array parse_array(Iterator begin, Iterator end, Iterator & iter)
{
	Array array;
	consume(begin, end, '[', iter);
	while (iter != end) {
		skip_whitespace(iter, end, iter);
		const auto value = parse_value(iter, end, iter);
		array.emplace_back(std::move(value));
		skip_whitespace(iter, end, iter);
		if (iter != end) {
			switch (*iter) {
			case ',': ++iter; break;
			case ']': ++iter; return array;
			default:
				throw ParseError("expected ',' or ']' token");
			}
		}
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator>
KeyValuePair parse_pair(Iterator begin, Iterator end, Iterator & iter)
{
	const auto key = parse_string(begin, end, iter);
	skip_whitespace(iter, end, iter);
	consume(iter, end, ':', iter);
	skip_whitespace(iter, end, iter);
	const auto value = parse_value(iter, end, iter);
	return std::make_pair(std::move(key), std::move(value));
}

template <class Iterator>
Object parse_object(Iterator begin, Iterator end, Iterator & iter)
{
	Object object;
	skip_whitespace(begin, end, iter);
	consume(iter, end, '{', iter);
	while (iter != end) {
		skip_whitespace(iter, end, iter);
		const auto pair = parse_pair(iter, end, iter);
		object.emplace(std::move(pair));
		skip_whitespace(iter, end, iter);
		if (iter != end) {
			switch (*iter) {
			case ',': ++iter; break;
			case '}': ++iter; return object;
			default:
				throw ParseError("expected ',' or '}' token");
			}
		}
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator>
int parse_sign_or(Iterator begin, Iterator end, int defvalue, Iterator & iter)
{
	iter = begin;
	if (iter != end) {
		switch (*iter) {
		case '-': ++iter; return -1;
		case '+': ++iter; return +1;
		}
	}
	return defvalue;
}

template <class Iterator>
bool try_parse_num(Iterator begin, Iterator end,
                   Iterator & iter, unsigned int & num)
{
	num = 0;
	iter = begin;
	for (; iter != end; ++iter) {
		const auto c = *iter;
		if (!std::isdigit(c))
			break;
		num *= 10;
		num += c - '0';
	}
	return iter != begin;
}

template <class Iterator>
unsigned int parse_num_or(Iterator begin, Iterator end,
                          unsigned int defvalue, Iterator & iter)
{
	unsigned int num;
	if (!try_parse_num(begin, end, iter, num))
		return defvalue;
	return num;
}

template <class Iterator>
bool try_parse_frac(Iterator begin, Iterator end, Iterator & iter, double & frac)
{
	if (!try_consume(begin, end, '.', iter))
		return false;
	frac = 0;
	double factor = 0.1;
	const auto start = iter;
	for (; iter != end; ++iter) {
		const auto c = *iter;
		if (!std::isdigit(c))
			break;
		frac += (c - '0') * factor;
		factor /= 10;
	}
	return iter != start;
}

template <class Iterator>
int parse_exp_or(Iterator begin, Iterator end, int defvalue, Iterator & iter)
{
	if (!try_consume(begin, end, 'e', iter)
	    && !try_consume(begin, end, 'E', iter))
		return defvalue;
	const auto sig = parse_sign_or(iter, end, 1, iter);
	unsigned int num;
	if (!try_parse_num(iter, end, iter, num))
		return defvalue;
	return sig * num;
}

template <class Iterator>
bool try_parse_float(Iterator begin, Iterator end, Iterator & iter, Float & value)
{
	const auto sig = parse_sign_or(begin, end, 1, iter);
	const auto num = parse_num_or(iter, end, 0, iter);
	double frac;
	if (!try_parse_frac(iter, end, iter, frac))
		return false;
	const auto exp = parse_exp_or(iter, end, 0, iter);
	value = sig * (num + frac) * std::pow(10, exp);
	return true;
}

template <class Iterator>
bool try_parse_int(Iterator begin, Iterator end, Iterator & iter, Int & value)
{
	const auto sig = parse_sign_or(begin, end, 1, iter);
	unsigned int num;
	if (!try_parse_num(iter, end, iter, num))
		return false;
	const auto exp = parse_exp_or(iter, end, 0, iter);
	value = sig * num * std::pow(10, exp);
	return true;
}

template <class Iterator>
Value parse_value(Iterator begin, Iterator end, Iterator & iter)
{
	if (begin == end)
		throw ParseError("unexpected end of input");
	switch (*begin) {
	case 'n':
		return parse_null(begin, end, iter);
	case 't':
		return parse_true(begin, end, iter);
	case 'f':
		return parse_false(begin, end, iter);
	case '"':
		return parse_string(begin, end, iter);
	case '[':
		return parse_array(begin, end, iter);
	case '{':
		return parse_object(begin, end, iter);
	default: {
		Iterator try_iter;
		float float_;
		if (try_parse_float(begin, end, try_iter, float_)) {
			iter = try_iter;
			return float_;
		}
		int int_;
		if (try_parse_int(begin, end, try_iter, int_)) {
			iter = try_iter;
			return int_;
		}
		throw ParseError("invalid value");
	} }
}

}

template <class Iterator>
Value parse(Iterator begin, Iterator end)
{
	return detail::parse_value(begin, end, begin);
}

template <typename CharT>
Value parse(std::basic_istream<CharT> & is)
{
	return parse(
		boost::spirit::basic_istream_iterator<CharT>(is),
		boost::spirit::basic_istream_iterator<CharT>()
	);
}

}

#endif
