#include "cmd.h"

#include <format>
#include <iostream>
#include <regex>

#include "fs.h"
#include "git_w.h"

namespace entropy::cmd {

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::tuple<std::string_view, std::optional<std::string_view>>> parse_cmd(std::string_view str_ln)
{
    // Trim whitespace
    auto start = str_ln.find_first_not_of(" \t\n\r");
    auto end = str_ln.find_last_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return Err<std::tuple<std::string_view, std::optional<std::string_view>>>("no command parsed");
    }
    auto trimmed = str_ln.substr(start, end - start + 1);

    // ^(?P<cmd>[^\s]+)(\s+(?P<args>.*))?$
    std::string input(trimmed);
    static const std::regex regex_cmd(R"(^(\S+)(\s+(.*))?$)");
    std::smatch match;

    if (!std::regex_match(input, match, regex_cmd)) {
        return Err<std::tuple<std::string_view, std::optional<std::string_view>>>("no command parsed");
    }

    // match[1] = cmd, match[3] = args (optional)
    // We need to return string_views into the original str_ln, so compute offsets
    auto cmd_start = static_cast<size_t>(match.position(1)) + start;
    auto cmd_len = static_cast<size_t>(match.length(1));
    auto str_cmd = str_ln.substr(cmd_start, cmd_len);

    std::optional<std::string_view> ostr_args;
    if (match[3].matched) {
        auto args_start = static_cast<size_t>(match.position(3)) + start;
        auto args_len = static_cast<size_t>(match.length(3));
        ostr_args = str_ln.substr(args_start, args_len);
    }

    return Ok<std::tuple<std::string_view, std::optional<std::string_view>>>(
        std::make_tuple(str_cmd, ostr_args));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::vector<std::string_view>> parse_tokens(std::string_view str_ln)
{
    // ^"[^"]+"|\\S+
    std::string input(str_ln);
    static const std::regex regex_token(R"(^"[^"]+"|[^\s]+)");
    std::vector<std::string_view> tokens;

    auto it = std::sregex_iterator(input.begin(), input.end(), regex_token);
    auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        auto pos = static_cast<size_t>(it->position());
        auto len = static_cast<size_t>(it->length());
        tokens.push_back(str_ln.substr(pos, len));
    }

    return Ok<std::vector<std::string_view>>(std::move(tokens));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<EntropyNode> parse_fn(std::string_view str_ln)
{
    // Trim
    auto start = str_ln.find_first_not_of(" \t\n\r");
    auto end = str_ln.find_last_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return Err<EntropyNode>("not valid fn spec found");
    }
    auto trimmed = str_ln.substr(start, end - start + 1);

    // ^((?P<domain>[^:]+):)?((?P<category>[^/]+)/)?(?P<fn>.*)$
    std::string input(trimmed);
    static const std::regex regex_fn(R"(^(([^:]+):)?(([^/]+)/)?(.*)$)");
    std::smatch match;

    if (!std::regex_match(input, match, regex_fn)) {
        return Err<EntropyNode>("not valid fn spec found");
    }

    // match[2] = domain, match[4] = category, match[5] = fn/name
    std::string_view str_domain = STR_DEFAULT_DOMAIN;
    if (match[2].matched && match[2].length() > 0) {
        // Need stable storage — use the trimmed view into str_ln
        auto domain_pos = static_cast<size_t>(match.position(2)) + start;
        auto domain_len = static_cast<size_t>(match.length(2));
        str_domain = str_ln.substr(domain_pos, domain_len);
    }

    std::optional<std::string_view> ostr_category;
    if (match[4].matched && match[4].length() > 0) {
        auto cat_pos = static_cast<size_t>(match.position(4)) + start;
        auto cat_len = static_cast<size_t>(match.length(4));
        ostr_category = str_ln.substr(cat_pos, cat_len);
    }

    if (!match[5].matched) {
        return Err<EntropyNode>("not valid fn spec found");
    }
    auto name_pos = static_cast<size_t>(match.position(5)) + start;
    auto name_len = static_cast<size_t>(match.length(5));
    auto str_nodename = str_ln.substr(name_pos, name_len);

    return Ok<EntropyNode>(EntropyNode(str_domain, ostr_category, str_nodename));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

// Helper: parse "<filepath>" to <fn_spec>
static Result<std::tuple<std::string_view, std::string_view>>
parse_file_to_fn(const std::optional<std::string_view>& ostr_args)
{
    using RetType = std::tuple<std::string_view, std::string_view>;

    if (!ostr_args) {
        return Err<RetType>("parse_file_to_fn: no args, skipping");
    }

    auto tokens_result = parse_tokens(*ostr_args);
    if (tokens_result.is_err()) {
        return Err<RetType>(std::format("parse_file_to_fn: failed to process: {}", tokens_result.error()));
    }

    auto& tokens = tokens_result.value();
    const std::string str_err = "parse_file_to_fn: invalid arguments";

    if (tokens.size() < 3) {
        return Err<RetType>(str_err);
    }

    auto it = tokens.begin();
    auto rstr_filepath = *it++;
    auto rstr_to = *it++;
    if (rstr_to != "to") {
        return Err<RetType>(str_err);
    }
    auto rstr_fn = *it;

    // Trim surrounding quotes
    auto trim_quotes = [](std::string_view sv) -> std::string_view {
        if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
            return sv.substr(1, sv.size() - 2);
        }
        return sv;
    };

    return Ok<RetType>(std::make_tuple(trim_quotes(rstr_filepath), trim_quotes(rstr_fn)));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

// node:new [DOMAIN]:[CATEGORY]/<NOTE>
ResultVoid node_new_markdown([[maybe_unused]] std::string_view str_cmd, const std::optional<std::string_view>& ostr_args)
{
    if (!ostr_args) {
        return Err("node_markdown_new: no args, skipping");
    }

    auto tokens_result = parse_tokens(*ostr_args);
    if (tokens_result.is_err()) {
        return Err(std::format("node_markdown_new: failed to process: {}", tokens_result.error()));
    }

    auto& tokens = tokens_result.value();
    if (tokens.empty()) {
        return Err("node_markdown_new: invalid arguments");
    }

    // Trim surrounding quotes from first token
    auto str_fn = tokens[0];
    if (str_fn.size() >= 2 && str_fn.front() == '"' && str_fn.back() == '"') {
        str_fn = str_fn.substr(1, str_fn.size() - 2);
    }
    std::cout << std::format("node_markdown_new: parse_tokens: str_fn[{}]\n", str_fn);

    auto node_result = parse_fn(str_fn);
    if (node_result.is_err()) {
        return Err(node_result.error());
    }
    auto& node = node_result.value();
    std::cout << std::format("node_markdown_new: domain[{}], name[{}]\n", node.get_str_domain(), node.get_str_name());

    auto metadata_result = fs::create_node(node);
    if (metadata_result.is_err()) {
        return Err(metadata_result.error());
    }
    auto& string_metadata = metadata_result.value();

    auto md_result = fs::create_markdown(node.get_str_name(), node);
    if (md_result.is_err()) {
        return Err(md_result.error());
    }
    auto& string_md = md_result.value();

    std::array<std::string_view, 2> arrstr_files = { string_metadata, string_md };
    auto commit_result = git_commit_files(node.get_path_absolute(), arrstr_files);
    if (commit_result.is_err()) {
        std::cerr << std::format("Git commit failed: {}\n", commit_result.error());
    }

    return Ok();
}

//-----------------------------------------------------------------------------------------------------------------------------------------

// file:new <FILEPATH> to [DOMAIN]:[CATEGORY]/<NODENAME>
ResultVoid file_new([[maybe_unused]] std::string_view str_cmd, const std::optional<std::string_view>& ostr_args)
{
    auto parsed = parse_file_to_fn(ostr_args);
    if (parsed.is_err()) return Err(parsed.error());
    auto [str_filepath, str_fn] = parsed.value();

    std::cout << std::format("file_new: parse_tokens: str_filepath[{}], str_fn[{}]\n", str_filepath, str_fn);

    auto node_result = parse_fn(str_fn);
    if (node_result.is_err()) return Err(node_result.error());
    auto node = std::move(node_result.value());

    std::cout << std::format("file_new: domain[{}], name[{}]\n", node.get_str_domain(), node.get_str_name());

    if (node.get_str_name() == ".") {
        auto stem_result = fs::file_stem(str_filepath);
        if (stem_result.is_err()) return Err(stem_result.error());

        std::optional<std::string_view> cat = std::nullopt;
        if (auto& ocat = node.get_ostring_category(); ocat) {
            cat = std::string_view(*ocat);
        }
        node = EntropyNode(node.get_str_domain(), cat, stem_result.value());
    }

    auto node_meta_result = fs::create_node(node);
    if (node_meta_result.is_err()) return Err(node_meta_result.error());
    auto& string_node_metadata = node_meta_result.value();

    auto move_result = fs::move_filepath_to(str_filepath, node, false);
    if (move_result.is_err()) return Err(move_result.error());
    auto [string_file_name, string_file_metadata] = move_result.value();

    std::array<std::string_view, 3> arrstr_files = { string_node_metadata, string_file_name, string_file_metadata };
    auto commit_result = git_commit_files(node.get_path_absolute(), arrstr_files);
    if (commit_result.is_err()) {
        return Err(std::format("file_new: git commit failed: {}", commit_result.error()));
    }

    return Ok();
}

//-----------------------------------------------------------------------------------------------------------------------------------------

// file:attach <FILEPATH> to [DOMAIN]:[CATEGORY]/<NODENAME>
ResultVoid file_attach([[maybe_unused]] std::string_view str_cmd, const std::optional<std::string_view>& ostr_args)
{
    auto parsed = parse_file_to_fn(ostr_args);
    if (parsed.is_err()) return Err(parsed.error());
    auto [str_filepath, str_fn] = parsed.value();

    std::cout << std::format("file_attach: parse_tokens: str_filepath[{}], str_fn[{}]\n", str_filepath, str_fn);

    auto node_result = parse_fn(str_fn);
    if (node_result.is_err()) return Err(node_result.error());
    auto& node = node_result.value();

    std::cout << std::format("file_attach: domain[{}], name[{}]\n", node.get_str_domain(), node.get_str_name());

    if (!std::filesystem::exists(node.get_path_absolute())) {
        return Err("file_attach: node target does not exist; aborting.");
    }

    auto move_result = fs::move_filepath_to(str_filepath, node, false);
    if (move_result.is_err()) return Err(move_result.error());
    auto [string_file_name, string_file_metadata] = move_result.value();

    std::array<std::string_view, 2> arrstr_files = { string_file_name, string_file_metadata };
    auto commit_result = git_commit_files(node.get_path_absolute(), arrstr_files);
    if (commit_result.is_err()) {
        return Err(std::format("file_attach: git commit failed: {}", commit_result.error()));
    }

    return Ok();
}

} // namespace entropy::cmd
