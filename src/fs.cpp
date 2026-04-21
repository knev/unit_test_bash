#include "fs.h"

#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <span>
#include <cassert>

#include <git2.h>

namespace entropy::fs {

namespace {

// Internal helper: create metadata file for a given filename inside path_dest
Result<std::string> file_metadata_new(const std::filesystem::path& path_dest, std::string_view str_filename,
                                      bool b_overwrite, std::span<const std::string> arrstr_tags,
                                      [[maybe_unused]] std::span<const std::pair<std::string, std::string>> arrstr2_props)
{
    auto str_metadata = std::format("{}.__.md", str_filename);
    auto path_metadata = path_dest / str_metadata;

    if (std::filesystem::exists(path_metadata) && !b_overwrite) {
        return Err<std::string>("file_metadata_new: file target exists; aborting.");
    }

    std::ofstream file_metadata(path_metadata);
    if (!file_metadata) {
        return Err<std::string>(std::format("file_metadata_new: file create failed: {}", path_metadata.string()));
    }

    std::string tags;
    for (auto& str_tag : arrstr_tags) {
        tags += std::format("\n  - {}", str_tag);
    }

    auto str_metadata_text = std::format(
R"(---
version: __entropy/v{}
tags: {}
- __entropy/metadata
redirect: "[[{}]]"
---
)", VERSION, tags, str_filename);

    file_metadata << str_metadata_text;
    if (!file_metadata) {
        return Err<std::string>("file_metadata_new: metadata write failed");
    }

    return Ok<std::string>(std::move(str_metadata));
}

} // anonymous namespace

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::string> create_node(const EntropyNode& node)
{
    if (std::filesystem::exists(node.get_path_absolute())) {
        return Err<std::string>(std::format("create_node: node target exists; aborting."));
    }

    std::error_code ec;
    std::filesystem::create_directories(node.get_path_absolute(), ec);
    if (ec) {
        return Err<std::string>(std::format("create_node: create node target: {}.", ec.message()));
    }
    std::cout << std::format("create_node: created node target: [{}].\n", node.get_str_name());

    // Initialize git repository
    {
        static struct Git2InitOnce { Git2InitOnce() { git_libgit2_init(); } } init;
        git_repository* repo = nullptr;
        if (git_repository_init(&repo, node.get_path_absolute().c_str(), 0) != 0) {
            const git_error* e = git_error_last();
            return Err<std::string>(std::format("create_node: Failed to initialize repository: {}",
                                                e ? e->message : "unknown"));
        }
        git_repository_free(repo);
        std::cout << "create_node: Repository initialized successfully.\n";
    }

    // Create metadata file
    auto string_metadata = std::format("__{}.__.md", node.get_str_name());
    auto path_metadata = node.get_path_absolute() / string_metadata;

    auto str_metadata_text = std::format(
R"(---
version: __entropy/v{}
tags:
  - __entropy/metadata
  - __domain/{}
---
)", VERSION, node.get_str_domain());

    std::ofstream file_metadata(path_metadata);
    if (!file_metadata) {
        return Err<std::string>(std::format("create_node: can not create file: {}", path_metadata.string()));
    }

    file_metadata << str_metadata_text;
    if (!file_metadata) {
        return Err<std::string>(std::format("create_node: Error writing to file: {}", path_metadata.string()));
    }

    std::cout << std::format("create_node: node created successfully: {}\n", node.get_string_relative());

    return Ok<std::string>(std::move(string_metadata));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::string> create_markdown(std::string_view str_filename, const EntropyNode& node)
{
    if (!std::filesystem::exists(node.get_path_absolute())) {
        return Err<std::string>("create_markdown: node target does NOT exist; aborting");
    }

    auto string_filename_md = std::format("{}.md", str_filename);
    auto path_nodename_md = node.get_path_absolute() / string_filename_md;

    std::ofstream file(path_nodename_md);
    if (!file) {
        return Err<std::string>(std::format("create_markdown: unable to create markdown: {}", path_nodename_md.string()));
    }

    return Ok<std::string>(std::move(string_filename_md));
}

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::string> file_stem(std::string_view str_filepath)
{
    auto pathbuf_filepath = std::filesystem::path(str_filepath);
    auto stem = pathbuf_filepath.stem();
    if (stem.empty()) {
        return Err<std::string>("file_stem: cannot extract filename from filepath");
    }
    return Ok<std::string>(stem.string());
}

//-----------------------------------------------------------------------------------------------------------------------------------------

Result<std::tuple<std::string, std::string>> move_filepath_to(std::string_view str_filepath, const EntropyNode& node, bool b_overwrite)
{
    assert(str_filepath.find('"') == std::string_view::npos && "move_filepath_to: string contains double quotes");

    auto pathbuf_filepath = std::filesystem::path(str_filepath);
    if (std::filesystem::is_directory(pathbuf_filepath)) {
        return Err<std::tuple<std::string, std::string>>("move_filepath_to: file to add is a directory");
    }

    auto filename = pathbuf_filepath.filename();
    if (filename.empty()) {
        return Err<std::tuple<std::string, std::string>>("move_filepath_to: cannot extract filename from filepath");
    }
    auto str_filepath_name = filename.string();

    auto pathbuf_dest = node.get_path_absolute() / str_filepath_name;
    std::cout << std::format("move_filepath_to: pathbuf_filepath[{}], pathbuf_dest[{}]\n",
                             pathbuf_filepath.string(), pathbuf_dest.string());

    if (std::filesystem::exists(pathbuf_dest) && !b_overwrite) {
        return Err<std::tuple<std::string, std::string>>("move_filepath_to: file target exists; aborting.");
    }

    std::error_code ec;
    std::filesystem::rename(pathbuf_filepath, pathbuf_dest, ec);
    if (ec) {
        return Err<std::tuple<std::string, std::string>>(std::format("move_filepath_to: moving file failed: {}", ec.message()));
    }

    auto metadata_result = file_metadata_new(node.get_path_absolute(), str_filepath_name, b_overwrite, {}, {});
    if (metadata_result.is_err()) {
        return Err<std::tuple<std::string, std::string>>(metadata_result.error());
    }

    return Ok<std::tuple<std::string, std::string>>(std::make_tuple(str_filepath_name, metadata_result.value()));
}

} // namespace entropy::fs
