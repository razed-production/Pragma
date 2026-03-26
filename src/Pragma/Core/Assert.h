#pragma once

#include "Pragma/Core/Log.h"

#include <stdexcept>

#define PRAGMA_ASSERT(condition, message)                                                     \
    do                                                                                        \
    {                                                                                         \
        if (!(condition))                                                                     \
        {                                                                                     \
            ::Pragma::Core::Log(::Pragma::Core::LogCategory::General, ::Pragma::Core::LogLevel::Error, (message)); \
            throw std::runtime_error((message));                                              \
        }                                                                                     \
    } while (false)
