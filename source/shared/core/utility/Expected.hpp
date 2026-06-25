#pragma once

#include <stdexcept>
#include <utility>
#include <variant>

#ifndef __cpp_lib_expected
namespace std
{
template<typename _Er>
class unexpected
{
public:
    explicit unexpected(_Er error)
        : error_{ std::move(error) }
    {
    }

    const _Er& error() const& noexcept { return error_; }
    _Er& error() & noexcept { return error_; }
    _Er&& error() && noexcept { return std::move(error_); }

private:
    _Er error_;
};

template<typename _Er>
unexpected(_Er) -> unexpected<_Er>;

template<typename _Tp, typename _Er>
class expected
{
public:
    expected(const _Tp& value)
        : data_{ value }
    {
    }

    expected(_Tp&& value)
        : data_{ std::move(value) }
    {
    }

    expected(const unexpected<_Er>& error)
        : data_{ error }
    {
    }

    expected(unexpected<_Er>&& error)
        : data_{ std::move(error) }
    {
    }

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<_Tp>(data_); }
    explicit operator bool() const noexcept { return has_value(); }

    _Tp& operator*() & { return std::get<_Tp>(data_); }
    const _Tp& operator*() const& { return std::get<_Tp>(data_); }
    _Tp&& operator*() && { return std::move(std::get<_Tp>(data_)); }

    _Tp& value() &
    {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<_Tp>(data_);
    }

    const _Tp& value() const&
    {
        if (!has_value()) {
            throw std::logic_error("bad expected access");
        }
        return std::get<_Tp>(data_);
    }

    _Er& error() & { return std::get<unexpected<_Er>>(data_).error(); }
    const _Er& error() const& { return std::get<unexpected<_Er>>(data_).error(); }
    _Er&& error() && { return std::move(std::get<unexpected<_Er>>(data_)).error(); }

private:
    std::variant<_Tp, unexpected<_Er>> data_;
};
} // namespace std
#endif
