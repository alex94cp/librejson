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

}
