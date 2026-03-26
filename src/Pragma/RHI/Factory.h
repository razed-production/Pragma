#pragma once

#include "Pragma/RHI/BackendType.h"

#include <memory>

namespace Pragma::RHI
{
class IDevice;

[[nodiscard]] std::unique_ptr<IDevice> CreateDevice(BackendType backend);
}
