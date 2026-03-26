#pragma once

#include <optional>
#include <string>
#include <utility>

namespace Pragma::Core
{
template <typename TValue>
class Result
{
public:
    static Result Success(TValue value)
    {
        return Result(std::move(value), "");
    }

    static Result Failure(std::string error)
    {
        return Result(std::nullopt, std::move(error));
    }

    [[nodiscard]] bool IsSuccess() const noexcept
    {
        return m_value.has_value();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return IsSuccess();
    }

    [[nodiscard]] const TValue& GetValue() const
    {
        return *m_value;
    }

    [[nodiscard]] TValue& GetValue()
    {
        return *m_value;
    }

    [[nodiscard]] const std::string& GetError() const noexcept
    {
        return m_error;
    }

private:
    Result(std::optional<TValue> value, std::string error)
        : m_value(std::move(value))
        , m_error(std::move(error))
    {
    }

private:
    std::optional<TValue> m_value;
    std::string m_error;
};
}
