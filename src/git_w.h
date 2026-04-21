#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "result.h"

namespace entropy {

ResultVoid git_commit_files(const std::filesystem::path& path_repo_local, std::span<const std::string_view> arrstr_files);

} // namespace entropy
