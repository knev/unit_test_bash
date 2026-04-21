#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <filesystem>
#include <format>
#include <iostream>

namespace entropy {

inline constexpr std::string_view STR_BASE = "/Users/dev/Entropy";

class EntropyNode {
    std::string string_domain_;
    std::optional<std::string> ostring_category_;
    std::string string_name_;
    std::string string_relative_;
    std::filesystem::path pathbuf_absolute_;

public:
    EntropyNode(std::string_view domain, std::optional<std::string_view> category, std::string_view name);

    [[nodiscard]] std::string_view get_str_domain() const { return string_domain_; }
    [[nodiscard]] const std::optional<std::string>& get_ostring_category() const { return ostring_category_; }
    [[nodiscard]] std::optional<std::string_view> get_str_category() const;
    [[nodiscard]] std::string_view get_str_name() const { return string_name_; }
    [[nodiscard]] const std::filesystem::path& get_path_absolute() const { return pathbuf_absolute_; }
    [[nodiscard]] const std::string& get_string_relative() const { return string_relative_; }
};

} // namespace entropy
