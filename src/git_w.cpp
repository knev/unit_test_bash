#include "git_w.h"

#include <format>
#include <iostream>
#include <git2.h>

namespace entropy {

// RAII wrapper for libgit2 init/shutdown
struct Git2Init {
    Git2Init()  { git_libgit2_init(); }
    ~Git2Init() { git_libgit2_shutdown(); }
};

ResultVoid git_commit_files(const std::filesystem::path& path_repo_local, std::span<const std::string_view> arrstr_files)
{
    static Git2Init git2_init;

    git_repository* repo = nullptr;
    if (git_repository_open(&repo, path_repo_local.c_str()) != 0) {
        const git_error* e = git_error_last();
        return Err(std::format("git_commit: failed to open repo: {}", e ? e->message : "unknown"));
    }

    git_index* index = nullptr;
    if (git_repository_index(&index, repo) != 0) {
        git_repository_free(repo);
        return Err("git_commit: failed to get index");
    }

    std::string commit_message = "files: ";
    for (auto& str_file : arrstr_files) {
        if (git_index_add_bypath(index, std::string(str_file).c_str()) != 0) {
            const git_error* e = git_error_last();
            git_index_free(index);
            git_repository_free(repo);
            return Err(std::format("git_commit: failed to add path: {}", e ? e->message : "unknown"));
        }
        commit_message += std::format("{};", str_file);
    }

    if (git_index_write(index) != 0) {
        git_index_free(index);
        git_repository_free(repo);
        return Err("git_commit: failed to write index");
    }

    git_oid tree_oid;
    if (git_index_write_tree(&tree_oid, index) != 0) {
        git_index_free(index);
        git_repository_free(repo);
        return Err("git_commit: failed to write tree");
    }

    git_tree* tree = nullptr;
    git_tree_lookup(&tree, repo, &tree_oid);

    git_signature* signature = nullptr;
    if (git_signature_default(&signature, repo) != 0) {
        git_tree_free(tree);
        git_index_free(index);
        git_repository_free(repo);
        return Err("git_commit: failed to get signature");
    }

    git_oid commit_oid;
    git_reference* head_ref = nullptr;
    int head_err = git_repository_head(&head_ref, repo);

    if (head_err == 0) {
        // Has parent commit
        git_commit* parent_commit = nullptr;
        const git_oid* head_oid = git_reference_target(head_ref);
        git_commit_lookup(&parent_commit, repo, head_oid);

        const git_commit* parents[] = { parent_commit };
        git_commit_create(&commit_oid, repo, "HEAD", signature, signature,
                          nullptr, commit_message.c_str(), tree, 1, parents);

        git_commit_free(parent_commit);
        git_reference_free(head_ref);
    } else {
        // Initial commit – no parent
        git_commit_create(&commit_oid, repo, "HEAD", signature, signature,
                          nullptr, commit_message.c_str(), tree, 0, nullptr);
    }

    git_signature_free(signature);
    git_tree_free(tree);
    git_index_free(index);
    git_repository_free(repo);

    return Ok();
}

} // namespace entropy
