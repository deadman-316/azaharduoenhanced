// Copyright Azahar Duo Enhanced contributors
// Licensed under GPLv3 or any later version
// Refer to the LICENSE.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"

namespace Vulkan {

class Instance;

/// Azahar Duo Enhanced: builds a plain-text diagnostics report describing the GPU, Vulkan
/// driver, supported extensions, pipeline cache UUID, CPU topology, memory and the settings
/// that affect shader compilation. Read-only; has no effect on emulation.
std::string BuildDuoDeviceReport(const Instance& instance, u64 title_id,
                                 std::size_t compile_worker_threads);

} // namespace Vulkan
