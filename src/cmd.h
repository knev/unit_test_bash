#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <tuple>
#include <vector>
#include <functional>

#include "global.h"
#include "result.h"

namespace entropy::cmd {

inline constexpr std::string_view STR_DEFAULT_DOMAIN = "_default";

using CmdFxn = std::function<ResultVoid(std::string_view, const std::optional<std::string_view>&)>;

Result<std::tuple<std::string_view, std::optional<std::string_view>>> parse_cmd(std::string_view str_ln);
Result<std::vector<std::string_view>> parse_tokens(std::string_view str_ln);
Result<EntropyNode> parse_fn(std::string_view str_ln);

ResultVoid node_new_markdown(std::string_view str_cmd, const std::optional<std::string_view>& ostr_args);
ResultVoid file_new(std::string_view str_cmd, const std::optional<std::string_view>& ostr_args);
ResultVoid file_attach(std::string_view str_cmd, const std::optional<std::string_view>& ostr_args);

} // namespace entropy::cmd
