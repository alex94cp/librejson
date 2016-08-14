#include <rejson/path.hpp>
#include <rejson/value.hpp>

#include <cctype>
#include <stdexcept>
#include <vector>

namespace rejson {

namespace {

auto make_object_accessor(std::string key)
{
	return [key = std::move(key)] (Value & v) -> Value * {
		auto & object = v.as_object();
		const auto iter = object.find(key);
		if (iter != object.end())
			return &iter->second;
		return nullptr;
	};
}

auto make_array_accessor(std::size_t index)
{
	return [index] (Value & v) -> Value * {
		auto & array = v.as_array();
		if (index < array.size())
			return &array[index];
		return nullptr;
	};
}

auto parse_json_path(detail::string_view path)
{
	std::size_t pos = 0;
	std::vector<std::function<Value *(Value &)>> resolvers;
	while (pos < path.size()) {
		switch (path[pos]) {
		case '[': {
			const auto endpos = path.find(']', pos + 1);
			if (endpos == detail::string_view::npos)
				throw std::invalid_argument("invalid json path");
			const std::size_t len = endpos - pos - 1;
			const auto key = path.substr(pos + 1, len);
			if (key.size() == 0)
				throw std::invalid_argument("invalid json path");
			if (std::isdigit(key.front())) {
				const auto index = std::stol(key.to_string());
				resolvers.emplace_back(make_array_accessor(index));
			} else {
				resolvers.emplace_back(make_object_accessor(key.to_string()));
			}
			pos = endpos + 1;
			break;
		}
		case '.':
			++pos;
		default: {
			auto endpos = path.find_first_of(".[", pos);
			const auto key = path.substr(pos, endpos - pos);
			if (key.size() == 0)
				throw std::invalid_argument("invalid json path");
			resolvers.emplace_back(make_object_accessor(key.to_string()));
			pos = endpos;
		} }
	}
	return resolvers;
}

}

Path::Path(detail::string_view path)
	: resolvers_ { parse_json_path(path) } {}

Path::Path(const char * path)
	: Path { detail::string_view { path } } {}

Value * Path::resolve(Value & v) const
{
	Value * result = &v;
	for (auto && resolve_one : resolvers_) {
		result = resolve_one(*result);
		if (!result)
			return nullptr;
	}
	return result;
}

const Value * Path::resolve(const Value & v) const
{
	return resolve(const_cast<Value &>(v));
}

Value * get(Value & root, const Path & path)
{
	return path.resolve(root);
}

const Value * get(const Value & root, const Path & path)
{
	return path.resolve(root);
}

detail::optional<Value> get(const Value && root, const Path & path)
{
	const auto value = path.resolve(root);
	if (!value)
		return detail::nullopt;
	return *value;
}

Value get_value_or(const Value & root, const Path & path, const Value & defval)
{
	const auto v = get(root, path);
	return v ? *v : defval;
}

}
