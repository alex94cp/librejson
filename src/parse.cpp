#include <rejson/parse.hpp>

namespace rejson {

Value parse(detail::string_view sv)
{
	return parse(sv.begin(), sv.end());
}

Value parse(detail::wstring_view sv)
{
	return parse(sv.begin(), sv.end());
}

Value parse(detail::u16string_view sv)
{
	return parse(sv.begin(), sv.end());
}

Value parse(detail::u32string_view sv)
{
	return parse(sv.begin(), sv.end());
}

namespace detail {

void encode_utf8(char32_t code_pt, std::ostringstream & os)
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

}

}
