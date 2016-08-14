#ifndef REJSON_DETAIL_OPTIONAL_HPP_
#define REJSON_DETAIL_OPTIONAL_HPP_

#ifdef __has_include
#	if __has_include(<optional>)
#		define rejson_have_std_optional 1
#		include <optional>
#	elif __has_include(<experimental/optional>)
#		define rejson_have_std_optional 1
#		define rejson_std_experimental_optional 1
#		include <experimental/optional>
#	endif
#endif

#if !rejson_have_std_optional
#	include <boost/optional.hpp>
#endif

namespace rejson { namespace detail {

#if rejson_have_std_optional
#	if rejson_std_experimental_optional
		using std::experimental::optional;
		using std::experimental::nullopt_t;
		using std::experimental::nullopt;
#	else
		using std::optional;
		using std::nullopt_t;
		using std::nullopt;
#	endif
#else
		using boost::optional;
		using nullopt_t = boost::none_t;
		const boost::none_t nullopt;
#endif

} }

#endif
