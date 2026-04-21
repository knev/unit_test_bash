#include <iostream>
#include <string>
#include <unordered_map>
#include <format>

#include "cmd.h"

int main()
{
    std::unordered_map<std::string_view, entropy::cmd::CmdFxn> commands;
    commands["node:new"]    = entropy::cmd::node_new_markdown;   // creates the node; adds a new markdown file
    commands["file:new"]    = entropy::cmd::file_new;            // creates the node; moves the file to the node
    commands["file:attach"] = entropy::cmd::file_attach;         // moves the file to the node; node must exist

    while (true) {
        std::cout << "> " << std::flush;

        std::string string_ln;
        if (!std::getline(std::cin, string_ln)) {
            // EOF (Ctrl+D)
            std::cout << "\nExiting.\n";
            break;
        }

        // Trim newline / whitespace
        auto str_cmd_view = std::string_view(string_ln);
        auto start = str_cmd_view.find_first_not_of(" \t\n\r");
        auto end = str_cmd_view.find_last_not_of(" \t\n\r");
        if (start == std::string_view::npos) continue;
        auto trimmed = str_cmd_view.substr(start, end - start + 1);

        std::cout << std::format("[{}]\n\n", trimmed);

        auto parse_result = entropy::cmd::parse_cmd(string_ln);
        if (parse_result.is_err()) {
            std::cout << std::format("main: parsing cmd fail: {}\n", parse_result.error());
            continue;
        }

        auto [str_cmd, ostr_args] = parse_result.value();
        std::cout << std::format("parse_cmd: str_cmd[{}]\n", str_cmd);

        auto it = commands.find(str_cmd);
        if (it != commands.end()) {
            auto result = it->second(str_cmd, ostr_args);
            if (result.is_err()) {
                std::cout << std::format("main: {}\n", result.error());
            }
        } else {
            std::cout << std::format("main: unknown command {}\n", str_cmd);
        }
    }

    return 0;
}
