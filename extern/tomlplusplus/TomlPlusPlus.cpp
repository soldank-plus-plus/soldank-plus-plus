module;

#include <sstream>
#include <string>
#include <string_view>

#include <toml++/toml.hpp>

export module Extern.TomlPlusPlus;

export namespace Toml
{
using Array = toml::array;
using Date = toml::date;
using DateTime = toml::date_time;
using Node = toml::node;
using ParseError = toml::parse_error;
using Table = toml::table;
using Time = toml::time;

template<typename T>
using NodeView = toml::node_view<T>;

template<typename T>
using Value = toml::value<T>;

Table Parse(std::string_view document)
{
    return toml::parse(document);
}

Table ParseFile(std::string_view file_path)
{
    return toml::parse_file(file_path);
}

std::string Format(const Table& table)
{
    std::ostringstream stream;
    stream << table;
    return stream.str();
}
} // namespace Toml
