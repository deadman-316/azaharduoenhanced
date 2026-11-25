// Copyright Azahar Duo Enhanced contributors
// Licensed under GPLv3 or any later version
// Refer to the LICENSE.txt file included.

#include <fstream>
#include <thread>
#include <fmt/format.h>
#include "common/scm_rev.h"
#include "common/settings.h"
#include "video_core/renderer_vulkan/vk_duo_report.h"
#include "video_core/renderer_vulkan/vk_instance.h"

#ifdef ANDROID
#include <sys/system_properties.h>
#endif

namespace Vulkan {

namespace {

std::string ReadFirstLine(const std::string& path) {
    std::ifstream file{path};
    std::string line;
    if (!file.is_open() || !std::getline(file, line)) {
        return "n/a";
    }
    return line;
}

std::string GetProp([[maybe_unused]] const char* name) {
#ifdef ANDROID
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get(name, value) > 0) {
        return value;
    }
#endif
    return "n/a";
}

} // Anonymous namespace

std::string BuildDuoDeviceReport(const Instance& instance, u64 title_id,
                                 std::size_t compile_worker_threads) {
    std::string out;
    const auto add = [&out](std::string_view key, const auto& value) {
        fmt::format_to(std::back_inserter(out), "{}: {}\n", key, value);
    };

    out += "Azahar Duo Enhanced - device diagnostics\n\n[build]\n";
    add("build", Common::g_build_fullname);
    add("scm_rev", Common::g_scm_rev);
    add("shader_cache_version", Common::g_shader_cache_version);
    add("title_id", fmt::format("{:016X}", title_id));

    out += "\n[device]\n";
    add("manufacturer", GetProp("ro.product.manufacturer"));
    add("model", GetProp("ro.product.model"));
    add("device", GetProp("ro.product.device"));
    add("soc_model", GetProp("ro.soc.model"));
    add("board_platform", GetProp("ro.board.platform"));
    add("android_release", GetProp("ro.build.version.release"));
    add("android_sdk", GetProp("ro.build.version.sdk"));
    add("build_fingerprint", GetProp("ro.build.fingerprint"));

    out += "\n[gpu]\n";
    add("device_name", instance.GetModelName());
    add("vendor_name", instance.GetVendorName());
    add("vendor_id", fmt::format("{:#06X}", instance.GetVendorID()));
    add("device_id", fmt::format("{:#010X}", instance.GetDeviceID()));
    add("driver_id", static_cast<u32>(instance.GetDriverID()));
    const u32 driver_version = instance.GetDriverVersion();
    add("driver_version_raw", fmt::format("{:#010X}", driver_version));
    add("driver_version_vk_decoded", fmt::format("{}.{}.{}", driver_version >> 22,
                                                 (driver_version >> 12) & 0x3ff,
                                                 driver_version & 0xfff));
    const u32 api_version = instance.ApiVersion();
    add("vulkan_api_version",
        fmt::format("{}.{}.{}", api_version >> 22, (api_version >> 12) & 0x3ff,
                    api_version & 0xfff));
    std::string uuid;
    for (const u8 byte : instance.GetPipelineCacheUUID()) {
        fmt::format_to(std::back_inserter(uuid), "{:02x}", byte);
    }
    add("pipeline_cache_uuid", uuid);

    out += "\n[vulkan features used by the pipeline cache]\n";
    add("extended_dynamic_state", instance.IsExtendedDynamicStateSupported());
    add("pipeline_creation_cache_control", instance.IsPipelineCreationCacheControlSupported());
    add("fragment_shader_barycentric", instance.IsFragmentShaderBarycentricSupported());
    add("fragment_shader_interlock", instance.IsFragmentShaderInterlockSupported());
    add("geometry_shaders_in_use", instance.UseGeometryShaders());
    add("needs_logic_op_emulation", instance.NeedsLogicOpEmulation());
    add("custom_border_color", instance.IsCustomBorderColorSupported());
    add("shader_clip_distance", instance.IsShaderClipDistanceSupported());
    add("timeline_semaphore", instance.IsTimelineSemaphoreSupported());

    out += "\n[settings]\n";
    add("async_shader_compilation", Settings::values.async_shader_compilation.GetValue());
    add("use_disk_shader_cache", Settings::values.use_disk_shader_cache.GetValue());
    add("spirv_shader_gen", Settings::values.spirv_shader_gen.GetValue());
    add("disable_spirv_optimizer", Settings::values.disable_spirv_optimizer.GetValue());
    add("shaders_accurate_mul", Settings::values.shaders_accurate_mul.GetValue());
    add("use_hw_shader", Settings::values.use_hw_shader.GetValue());
    add("async_presentation", Settings::values.async_presentation.GetValue());
    add("resolution_factor", Settings::values.resolution_factor.GetValue());

    out += "\n[cpu]\n";
    const u32 logical = std::thread::hardware_concurrency();
    add("hardware_concurrency", logical);
    add("compile_worker_threads_per_pool", compile_worker_threads);
    add("online", ReadFirstLine("/sys/devices/system/cpu/online"));
    for (u32 cpu = 0; cpu < logical && cpu < 32; cpu++) {
        const std::string base = fmt::format("/sys/devices/system/cpu/cpu{}/", cpu);
        fmt::format_to(std::back_inserter(out),
                       "cpu{}: max_khz={} min_khz={} capacity={} cluster_cpus={}\n", cpu,
                       ReadFirstLine(base + "cpufreq/cpuinfo_max_freq"),
                       ReadFirstLine(base + "cpufreq/cpuinfo_min_freq"),
                       ReadFirstLine(base + "cpu_capacity"),
                       ReadFirstLine(base + "cpufreq/related_cpus"));
    }

    out += "\n[memory] (/proc/meminfo)\n";
    {
        std::ifstream meminfo{"/proc/meminfo"};
        std::string line;
        while (meminfo.is_open() && std::getline(meminfo, line)) {
            if (line.starts_with("MemTotal") || line.starts_with("MemFree") ||
                line.starts_with("MemAvailable") || line.starts_with("SwapTotal") ||
                line.starts_with("SwapFree")) {
                out += line;
                out += '\n';
            }
        }
    }

    const auto extensions = instance.GetAvailableExtensions();
    fmt::format_to(std::back_inserter(out), "\n[vulkan device extensions] ({})\n",
                   extensions.size());
    for (const std::string& extension : extensions) {
        out += extension;
        out += '\n';
    }

    return out;
}

} // namespace Vulkan
