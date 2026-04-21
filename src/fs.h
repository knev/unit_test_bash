#pragma once

#include <string>
#include <string_view>
#include <tuple>

#include "global.h"
#include "result.h"

namespace entropy::fs {

inline constexpr std::string_view VERSION = "0.2";

Result<std::string> create_node(const EntropyNode& node);
Result<std::string> create_markdown(std::string_view str_filename, const EntropyNode& node);
Result<std::string> file_stem(std::string_view str_filepath);
Result<std::tuple<std::string, std::string>> move_filepath_to(std::string_view str_filepath, const EntropyNode& node, bool b_overwrite);

} // namespace entropy::fs
