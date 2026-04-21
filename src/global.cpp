#include "global.h"

namespace entropy {

EntropyNode::EntropyNode(std::string_view domain, std::optional<std::string_view> category, std::string_view name)
    : string_domain_(domain)
    , ostring_category_(category ? std::optional<std::string>(std::string(*category)) : std::nullopt)
    , string_name_(name)
{
    auto path_base = std::filesystem::path(STR_BASE);

    if (category) {
        string_relative_ = std::format("{}/{}", *category, name);
        pathbuf_absolute_ = path_base / string_relative_;
    } else {
        string_relative_ = std::string(name);
        pathbuf_absolute_ = path_base / name;
    }

    std::cout << std::format("get_node: string_relative_path[{}], pathbuf_absolute_path[{}]\n",
                             string_relative_, pathbuf_absolute_.string());
}

std::optional<std::string_view> EntropyNode::get_str_category() const {
    if (ostring_category_) {
        return std::string_view(*ostring_category_);
    }
    return std::nullopt;
}

} // namespace entropy
