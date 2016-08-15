#ifndef REJSON_PARSE_HPP_
#define REJSON_PARSE_HPP_

#include <rejson/value.hpp>
#include <rejson/detail/string_view.hpp>

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

namespace rejson {

class REJSON_EXPORT ParseError : public std::runtime_error
{
	using runtime_error::runtime_error;
};

REJSON_EXPORT Value parse(detail::string_view sv);
REJSON_EXPORT Value parse(detail::wstring_view sv);
REJSON_EXPORT Value parse(detail::u16string_view sv);
REJSON_EXPORT Value parse(detail::u32string_view sv);

template <class Iterator>
Value parse(Iterator begin, Iterator end);

template <typename Char>
Value parse(std::basic_istream<Char> & is);

namespace detail {

template <class Iterator>
Value parse_value(Iterator & begin, Iterator end);

template <class Iterator>
using char_type = typename std::iterator_traits<Iterator>::value_type;

template <class Iterator, typename Char>
bool try_next_char(Iterator & begin, Iterator end, Char & chr)
{
	if (begin == end)
		return false;
	chr = *begin++;
	return true;
}

template <class Iterator>
auto next_char(Iterator & begin, Iterator end)
{
	char_type<Iterator> chr;
	if (!try_next_char(begin, end, chr))
		throw ParseError("unexpected end of input");
	return chr;
}

template <class Iterator>
char_type<Iterator> peek_char(Iterator begin, Iterator end)
{
	if (begin == end)
		throw ParseError("unexpected end of input");
	return *begin;
}

template <class Iterator, typename Char>
void consume(Iterator & begin, Iterator end, Char token)
{
	using namespace std::literals::string_literals;
	const auto chr = peek_char(begin, end);
	if (chr != token)
		throw ParseError("expected '"s + token + "' token");
	++begin;
}

template <class Iterator, typename Char>
bool try_consume(Iterator & begin, Iterator end, Char token)
{
	if (begin != end && *begin == token)
		return ++begin, true;
	return false;
}

template <class Iterator, typename Char>
bool try_consume(Iterator & begin, Iterator end, const Char * tokens)
{
	Iterator try_iter = begin;
	while (const auto t = *tokens++) {
		if (!try_consume(try_iter, end, t))
			return false;
	}
	begin = try_iter;
	return true;
}

template <class Iterator>
void skip_whitespace(Iterator & begin, Iterator end)
{
	for (; begin != end; ++begin) {
		if (!std::isspace(*begin))
			break;
	}
}

template <class Iterator>
Null parse_null(Iterator & begin, Iterator end)
{
	if (!try_consume(begin, end, "null"))
		throw ParseError("invalid value");
	return nullptr;
}

template <class Iterator>
Bool parse_true(Iterator & begin, Iterator end)
{
	if (!try_consume(begin, end, "true"))
		throw ParseError("invalid value");
	return true;
}

template <class Iterator>
Bool parse_false(Iterator & begin, Iterator end)
{
	if (!try_consume(begin, end, "false"))
		throw ParseError("invalid value");
	return false;
}

template <class Iterator>
bool try_parse_xdigit(Iterator & begin, Iterator end, std::uint8_t & xdigit)
{
	char_type<Iterator> chr;
	Iterator try_iter = begin;
	if (!try_next_char(try_iter, end, chr))
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
	begin = try_iter;
	return true;
}

template <class Iterator>
bool try_parse_codept(Iterator & begin, Iterator end, char16_t & code_pt)
{
	Iterator try_iter = begin;
	std::uint8_t d0, d1, d2, d3;
	if (!try_consume(try_iter, end, "\\u")
	    || !try_parse_xdigit(try_iter, end, d0)
	    || !try_parse_xdigit(try_iter, end, d1)
	    || !try_parse_xdigit(try_iter, end, d2)
	    || !try_parse_xdigit(try_iter, end, d3))
		return false;
	code_pt = d0 << ((CHAR_BIT / 2) * 3)
	        | d1 << ((CHAR_BIT / 2) * 2)
	        | d2 << ((CHAR_BIT / 2) * 1)
	        | d3 << ((CHAR_BIT / 2) * 0);
	begin = try_iter;
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
char_type<Iterator> parse_escaped(Iterator & begin, Iterator end)
{
	consume(begin, end, '\\');
	const auto chr = next_char(begin, end);
	switch (chr) {
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	default:  return chr;
	}
}

inline void encode_utf8(char32_t code_pt, std::ostringstream & os)
{
	if (code_pt < 0x80) {
		os.put(code_pt & 0xff);
	} else if (code_pt < 0x8000) {
		os.put((code_pt >> 6) | 0xc0);
		os.put((code_pt & 0x3f) | 0x80);
	} else if (code_pt < 0x1000) {
		os.put((code_pt >> 12) | 0xe0);
		os.put(((code_pt >> 6) & 0x3f) | 0x80);
		os.put((code_pt & 0x3f) | 0x80);
	} else {
		os.put((code_pt >> 18) | 0xf0);
		os.put(((code_pt >> 12) & 0x3f) | 0x80);
		os.put(((code_pt >> 6) & 0x3f) | 0x80);
		os.put((code_pt & 0x3f) | 0x80);
	}
}

template <class Iterator>
String parse_string(Iterator & begin, Iterator end)
{
	long last_code_pt = -1;
	std::ostringstream oss;
	consume(begin, end, '"');
	while (begin != end) {
		switch (const auto chr = *begin) {
		case '"':
			if (last_code_pt != -1)
				encode_utf8(last_code_pt, oss);
			return ++begin, oss.str();
		case '\\': {
			char16_t code_pt;
			if (try_parse_codept(begin, end, code_pt)) {
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
				const auto esc = parse_escaped(begin, end);
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
			++begin;
		}
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator>
Array parse_array(Iterator & begin, Iterator end)
{
	Array array;
	consume(begin, end, '[');
	char_type<Iterator> last_token = '[';
	while (begin != end) {
		skip_whitespace(begin, end);
		const auto chr = *begin;
		switch (chr) {
		case ',':
			if (last_token == '[' || last_token == ',')
				throw ParseError("unexpected ',' token");
			++begin; break;
		case ']':
			if (last_token == ',')
				throw ParseError("unexpected ',' token");
			return ++begin, array;
		default:
			if (last_token != '[' && last_token != ',')
				throw ParseError("expected ',' or ']' token");
			array.emplace_back(parse_value(begin, end));
		}
		last_token = chr;
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator>
KeyValuePair parse_pair(Iterator & begin, Iterator end)
{
	const auto key = parse_string(begin, end);
	skip_whitespace(begin, end);
	consume(begin, end, ':');
	skip_whitespace(begin, end);
	const auto value = parse_value(begin, end);
	return std::make_pair(std::move(key), std::move(value));
}

template <class Iterator>
Object parse_object(Iterator & begin, Iterator end)
{
	Object object;
	consume(begin, end, '{');
	char_type<Iterator> last_token = '{';
	while (begin != end) {
		skip_whitespace(begin, end);
		const auto chr = *begin;
		switch (*begin) {
		case ',':
			if (last_token == '{' || last_token == ',')
				throw ParseError("unexpected ',' token");
			++begin; break;
		case '}':
			if (last_token == ',')
				throw ParseError("unexpected ',' token");
			return ++begin, object;
		default:
			if (last_token != '{' && last_token != ',')
				throw ParseError("expected ',' or '}' token");
			object.emplace(parse_pair(begin, end));
		}
		last_token = chr;
	}
	throw ParseError("unexpected end of input");
}

template <class Iterator, typename Sign>
Sign parse_sign_or(Iterator & begin, Iterator end, Sign defvalue)
{
	if (begin != end) {
		switch (*begin) {
		case '-': ++begin; return -1;
		case '+': ++begin; return +1;
		}
	}
	return defvalue;
}

template <class Iterator, typename Num>
bool try_parse_num(Iterator & begin, Iterator end, Num & num)
{
	Num result = 0;
	Iterator try_iter;
	for (try_iter = begin; try_iter != end; ++try_iter) {
		const auto chr = *try_iter;
		if (!std::isdigit(chr))
			break;
		result *= 10;
		result += chr - '0';
	}
	if (try_iter == begin)
		return false;
	begin = try_iter;
	num = result;
	return true;
}

template <class Iterator, typename Frac>
bool try_parse_frac(Iterator & begin, Iterator end, Frac & frac)
{
	Iterator try_iter = begin;
	Frac factor = 0.1, result = 0;
	if (!try_consume(try_iter, end, '.'))
		return false;
	const auto start_iter = try_iter;
	while (try_iter != end) {
		const auto chr = *try_iter++;
		if (!std::isdigit(chr))
			break;
		result += (chr - '0') * factor;
		factor /= 10;
	}
	if (try_iter == start_iter)
		return false;
	begin = try_iter;
	frac = result;
	return true;
}

template <class Iterator, typename Exp>
bool try_parse_exp(Iterator & begin, Iterator end, Exp & exp)
{
	Exp num;
	Iterator try_iter = begin;
	if (!try_consume(try_iter, end, 'e')
	    && !try_consume(try_iter, end, 'E'))
		return false;
	const int sig = parse_sign_or(try_iter, end, +1);
	if (!try_parse_num(try_iter, end, num))
		return false;
	begin = try_iter;
	exp = sig * num;
	return true;
}

template <typename Char>
constexpr bool is_valid_number_start(Char chr)
{
	return std::isdigit(chr) || chr == '-' || chr == '.';
}

template <class Iterator>
Value parse_number(Iterator & begin, Iterator end)
{
	Int dec = 0, exp = 0; Real frac = 0;
	if (!is_valid_number_start(peek_char(begin, end)))
		throw ParseError("invalid value");
	const long sig = parse_sign_or(begin, end, +1);
	const auto dec_start = peek_char(begin, end);
	const bool has_dec = try_parse_num(begin, end, dec);
	const bool has_frac = try_parse_frac(begin, end, frac);
	const bool has_exp = try_parse_exp(begin, end, exp);
	const bool is_real = has_frac || has_exp;
	if (!is_real) {
		if (!has_dec)
			throw ParseError("invalid value");
		if (dec_start == '0' && dec != 0)
			throw ParseError("invalid value");
		return static_cast<Int>(sig * dec);
	}
	return sig * (dec + frac) * std::pow(10, exp);
}

template <class Iterator>
Value parse_value(Iterator & begin, Iterator end)
{
	skip_whitespace(begin, end);
	switch (peek_char(begin, end)) {
	case 'n': return parse_null(begin, end);
	case 't': return parse_true(begin, end);
	case 'f': return parse_false(begin, end);
	case '"': return parse_string(begin, end);
	case '[': return parse_array(begin, end);
	case '{': return parse_object(begin, end);
	default:  return parse_number(begin, end);
	}
}

}

template <class Iterator>
Value parse(Iterator begin, Iterator end)
{
	return detail::parse_value(begin, end);
}

template <typename Char>
Value parse(std::basic_istream<Char> & is)
{
	return parse(std::istream_iterator<Char>(is),
	             std::istream_iterator<Char>());
}

}

#endif
