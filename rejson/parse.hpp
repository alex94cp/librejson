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
#include <iterator>
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

template <class Iterator>
using char_type = typename std::iterator_traits<Iterator>::value_type;

template <class Iterator, typename CharT>
bool try_next_char(Iterator begin, Iterator end, Iterator & iter, CharT & chr)
{
	if (begin == end)
		return false;
	iter = begin;
	chr = *iter++;
	return true;
}

template <class Iterator>
auto next_char(Iterator begin, Iterator end, Iterator & iter)
{
	char_type<Iterator> chr;
	if (!try_next_char(begin, end, iter, chr))
		throw ParseError("unexpected end of input");
	return chr;
}

template <class Iterator, typename CharT>
void consume(Iterator begin, Iterator end, CharT token, Iterator & iter)
{
	using namespace std::literals::string_literals;
	const auto chr = next_char(begin, end, iter);
	if (chr != token)
		throw ParseError("expected '"s + token + "' token");
}

template <class Iterator, typename CharT>
bool try_consume(Iterator begin, Iterator end, CharT token, Iterator & iter)
{
	Iterator try_iter;
	char_type<Iterator> chr;
	if (!try_next_char(begin, end, try_iter, chr))
		return false;
	if (chr != token)
		return false;
	iter = try_iter;
	return true;
}

template <class Iterator, typename CharT>
bool try_consume(Iterator begin, Iterator end,
                 const CharT * tokens, Iterator & iter)
{
	Iterator try_iter = begin;
	while (const char t = *tokens++) {
		char_type<Iterator> chr;
		if (!try_next_char(try_iter, end, try_iter, chr))
			return false;
		if (chr != t)
			return false;
	}
	iter = try_iter;
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
	Iterator try_iter;
	char_type<Iterator> chr;
	if (!try_next_char(begin, end, try_iter, chr))
		return false;
	if (std::isdigit(chr)) {
		xdigit = chr - '0';
	} else if (chr >= 'a' && chr <= 'f') {
		xdigit = 10 + (chr - 'a');
	} else if (chr >= 'A' && chr <= 'F') {
		xdigit = 10 + (chr - 'A');
	} else {
		return false;
	}
	iter = try_iter;
	return true;
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

constexpr bool is_codept_group(char16_t first, char16_t second)
{
	return in_range(first, 0xd800, 0xdbff)
	    && in_range(second, 0xdc00, 0xdfff);
}

template <class Iterator>
char parse_escaped(Iterator begin, Iterator end, Iterator & iter)
{
	consume(begin, end, '\\', iter);
	const auto chr = next_char(iter, end, iter);
	switch (chr) {
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	default:  return chr;
	}
}

template <class Iterator>
String parse_string(Iterator begin, Iterator end, Iterator & iter)
{
	long last_code_pt = -1;
	std::ostringstream oss;
	consume(begin, end, '"', iter);
	while (iter != end) {
		switch (const char chr = *iter) {
		case '"':
			if (last_code_pt != -1)
				encode_utf8(last_code_pt, oss);
			return ++iter, oss.str();
		case '\\': {
			char16_t code_pt;
			if (try_parse_codept(iter, end, iter, code_pt)) {
				if (is_codept_group(last_code_pt, code_pt)) {
					const auto hi = last_code_pt - 0xd800;
					const auto lo = code_pt - 0xdc00 + 0x10000;
					encode_utf8(hi << 10 | lo, oss);
					last_code_pt = -1;
				} else {
					if (last_code_pt != -1)
						encode_utf8(last_code_pt, oss);
					last_code_pt = code_pt;
				}
			} else {
				const auto esc = parse_escaped(iter, end, iter);
				if (last_code_pt != -1)
					encode_utf8(last_code_pt, oss);
				last_code_pt = -1;
				oss.put(esc);
			}
			break;
		}
		default:
			if (last_code_pt != -1)
				encode_utf8(last_code_pt, oss);
			last_code_pt = -1;
			if (std::iscntrl(chr))
				throw ParseError("unescaped data in string");
			oss.put(chr);
			++iter;
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
	const auto key = parse_value(begin, end, iter);
	if (!key.is_string())
		throw ParseError("invalid key type");
	skip_whitespace(iter, end, iter);
	consume(iter, end, ':', iter);
	skip_whitespace(iter, end, iter);
	const auto value = parse_value(iter, end, iter);
	return std::make_pair(
		std::move(key).as_string(), std::move(value)
	);
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

template <class Iterator, typename Num>
bool try_parse_num(Iterator begin, Iterator end, Iterator & iter, Num & num)
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

template <class Iterator, typename Frac>
bool try_parse_frac(Iterator begin, Iterator end, Iterator & iter, Frac & frac)
{
	if (!try_consume(begin, end, '.', iter))
		return false;
	double factor = 0.1;
	const auto start = iter;
	for (frac = 0; iter != end; ++iter) {
		const auto c = *iter;
		if (!std::isdigit(c))
			break;
		frac += (c - '0') * factor;
		factor /= 10;
	}
	return iter != start;
}

template <class Iterator, typename Exp>
bool try_parse_exp(Iterator begin, Iterator end, Iterator & iter, Exp & exp)
{
	Iterator try_iter;
	if (!try_consume(begin, end, 'e', try_iter)
	    && !try_consume(begin, end, 'E', try_iter))
		return false;
	unsigned int num;
	const int sig = parse_sign_or(try_iter, end, +1, try_iter);
	if (!try_parse_num(try_iter, end, try_iter, num))
		return false;
	iter = try_iter;
	exp = sig * num;
	return true;
}

template <typename CharT>
constexpr bool is_valid_number_start(CharT chr)
{
	return std::isdigit(chr) || chr == '-' || chr == '.';
}

template <class Iterator>
Value parse_number(Iterator begin, Iterator end, Iterator & iter)
{
	iter = begin;
	if (iter == end)
		throw ParseError("unexpected enf of input");
	if (!is_valid_number_start(*iter))
		throw ParseError("invalid value");
	unsigned int dec = 0; double frac = 0; int exp = 0;
	const long sig = parse_sign_or(begin, end, +1, iter);
	const bool has_dec = try_parse_num(iter, end, iter, dec);
	const bool has_frac = try_parse_frac(iter, end, iter, frac);
	const bool has_exp = try_parse_exp(iter, end, iter, exp);
	const bool is_real = has_frac || has_exp;
	if (!is_real && !has_dec)
		throw ParseError("invalid value");
	if (!is_real)
		return static_cast<Int>(sig * dec);
	return sig * (dec + frac) * std::pow(10, exp);
}

template <class Iterator>
Value parse_value(Iterator begin, Iterator end, Iterator & iter)
{
	if (begin == end)
		throw ParseError("unexpected end of input");
	switch (*begin) {
	case 'n': return parse_null(begin, end, iter);
	case 't': return parse_true(begin, end, iter);
	case 'f': return parse_false(begin, end, iter);
	case '"': return parse_string(begin, end, iter);
	case '[': return parse_array(begin, end, iter);
	case '{': return parse_object(begin, end, iter);
	default:  return parse_number(begin, end, iter);
	}
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
