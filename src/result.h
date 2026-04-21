#pragma once

#include <string>
#include <stdexcept>
#include <optional>
#include <utility>

namespace entropy {

// Lightweight Result<T, String> analogous to Rust's Result type.
template <typename T>
class Result {
    bool ok_;
    std::optional<T> value_;
    std::string error_;

    Result(bool ok, std::optional<T> val, std::string err)
        : ok_(ok), value_(std::move(val)), error_(std::move(err)) {}

public:
    [[nodiscard]] bool is_ok() const { return ok_; }
    [[nodiscard]] bool is_err() const { return !ok_; }

    [[nodiscard]] T& value() { return *value_; }
    [[nodiscard]] const T& value() const { return *value_; }
    [[nodiscard]] const std::string& error() const { return error_; }

    T unwrap() {
        if (!ok_) throw std::runtime_error(error_);
        return std::move(*value_);
    }

    static Result ok(T val) {
        return Result(true, std::move(val), {});
    }

    static Result err(std::string msg) {
        return Result(false, std::nullopt, std::move(msg));
    }
};

// Specialization for monostate (void result)
template <>
class Result<std::monostate> {
    bool ok_;
    std::string error_;

    Result(bool ok, std::string err) : ok_(ok), error_(std::move(err)) {}

public:
    [[nodiscard]] bool is_ok() const { return ok_; }
    [[nodiscard]] bool is_err() const { return !ok_; }
    [[nodiscard]] const std::string& error() const { return error_; }

    static Result ok(std::monostate = {}) { return Result(true, {}); }
    static Result err(std::string msg) { return Result(false, std::move(msg)); }
};

using ResultVoid = Result<std::monostate>;

inline ResultVoid Ok() { return ResultVoid::ok(); }
inline ResultVoid Err(std::string msg) { return ResultVoid::err(std::move(msg)); }

template <typename T>
Result<T> Ok(T val) { return Result<T>::ok(std::move(val)); }

template <typename T>
Result<T> Err(std::string msg) { return Result<T>::err(std::move(msg)); }

} // namespace entropy
