#ifndef REJSON_DETAIL_STRING_VIEW_HPP_
#define REJSON_DETAIL_STRING_VIEW_HPP_

#ifdef __has_include
#	if __has_include(<string_view>)
#		define rejson_have_std_string_view 1
#		include <string_view>
#	elif __has_include(<experimental/string_view>)
#		define rejson_have_std_string_view 1
#		define rejson_std_experimental_string_view 1
#		include <experimental/string_view>
#	endif
#endif

#if !rejson_have_std_string_view
#	include <boost/utility/string_ref.hpp>
#endif

namespace rejson { namespace detail {

#if rejson_have_std_string_view
#	if rejson_std_experimental_string_view
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

} }

#endif
