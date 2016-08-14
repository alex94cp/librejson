#ifndef REJSON_PATH_HPP_
#define REJSON_PATH_HPP_

#include <rejson/value.hpp>
#include <rejson/detail/optional.hpp>
#include <rejson/detail/string_view.hpp>

#include <functional>
#include <vector>

namespace rejson {

class REJSON_EXPORT Path
{
public:
	Path(const char * path);
	Path(detail::string_view path);

	Value * resolve(Value & v) const;
	const Value * resolve(const Value & v) const;

private:
	std::vector<std::function<Value * (Value &)>> resolvers_;
};

REJSON_EXPORT
Value * get(Value & root, const Path & path);

REJSON_EXPORT
const Value * get(const Value & root, const Path & path);

REJSON_EXPORT
detail::optional<Value> get(const Value && root, const Path & path);

REJSON_EXPORT
Value get_value_or(const Value & root, const Path & path, const Value & defval);

}

#endif
