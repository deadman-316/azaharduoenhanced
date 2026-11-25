list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

include(GenerateBuildInfo)
generate_build_info()

# The variable SRC_DIR must be passed into the script (since it uses the current build directory for all values of CMAKE_*_DIR)
set(VIDEO_CORE "${CMAKE_SOURCE_DIR}/src/video_core")
set(HASH_FILES
    "${VIDEO_CORE}/renderer_opengl/gl_shader_disk_cache.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_disk_cache.h"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_util.cpp"
    "${VIDEO_CORE}/renderer_opengl/gl_shader_util.h"
    "${VIDEO_CORE}/renderer_vulkan/vk_shader_util.cpp"
    "${VIDEO_CORE}/renderer_vulkan/vk_shader_util.h"
    "${VIDEO_CORE}/renderer_vulkan/vk_shader_disk_cache.cpp"
    "${VIDEO_CORE}/renderer_vulkan/vk_shader_disk_cache.h"
    "${VIDEO_CORE}/renderer_vulkan/vk_pipeline_cache.cpp"
    "${VIDEO_CORE}/renderer_vulkan/vk_pipeline_cache.h"
    "${VIDEO_CORE}/shader/generator/glsl_fs_shader_gen.cpp"
    "${VIDEO_CORE}/shader/generator/glsl_fs_shader_gen.h"
    "${VIDEO_CORE}/shader/generator/glsl_shader_decompiler.cpp"
    "${VIDEO_CORE}/shader/generator/glsl_shader_decompiler.h"
    "${VIDEO_CORE}/shader/generator/glsl_shader_gen.cpp"
    "${VIDEO_CORE}/shader/generator/glsl_shader_gen.h"
    "${VIDEO_CORE}/shader/generator/pica_fs_config.cpp"
    "${VIDEO_CORE}/shader/generator/pica_fs_config.h"
    "${VIDEO_CORE}/shader/generator/profile.h"
    "${VIDEO_CORE}/shader/generator/shader_gen.cpp"
    "${VIDEO_CORE}/shader/generator/shader_gen.h"
    "${VIDEO_CORE}/shader/generator/shader_uniforms.cpp"
    "${VIDEO_CORE}/shader/generator/shader_uniforms.h"
    "${VIDEO_CORE}/shader/generator/spv_fs_shader_gen.cpp"
    "${VIDEO_CORE}/shader/generator/spv_fs_shader_gen.h"
    "${VIDEO_CORE}/shader/shader.cpp"
    "${VIDEO_CORE}/shader/shader.h"
    "${VIDEO_CORE}/pica/regs_framebuffer.h"
    "${VIDEO_CORE}/pica/regs_lighting.h"
    "${VIDEO_CORE}/pica/regs_pipeline.h"
    "${VIDEO_CORE}/pica/regs_rasterizer.h"
    "${VIDEO_CORE}/pica/regs_shader.h"
    "${VIDEO_CORE}/pica/regs_texturing.h"
    "${VIDEO_CORE}/pica/regs_internal.cpp"
    "${VIDEO_CORE}/pica/regs_internal.h"
)
set(COMBINED "")
foreach (F IN LISTS HASH_FILES)
    file(READ ${F} TMP)
    set(COMBINED "${COMBINED}${TMP}")
endforeach()
# The free camera option changes the shaders, but not the sources, so hash as well.
if (ENABLE_FREECAM)
    set(COMBINED "${COMBINED}ENABLE_FREECAM")
endif()
string(MD5 SHADER_CACHE_VERSION "${COMBINED}")

# Azahar Duo Enhanced: several of the files hashed above carry instrumentation and cache
# management changes that do not alter the generated shaders. To keep shader caches
# interchangeable with the upstream release this fork is based on, the hash of the pristine
# upstream sources is pinned in duo/upstream_shader_cache_version.txt.
# - Regenerate that file from the untouched upstream tag on every rebase.
# - Delete it if this fork ever changes what the shader generators emit.
set(DUO_PINNED_HASH_FILE "${CMAKE_SOURCE_DIR}/duo/upstream_shader_cache_version.txt")
if (EXISTS "${DUO_PINNED_HASH_FILE}")
    file(STRINGS "${DUO_PINNED_HASH_FILE}" DUO_PINNED_HASH LIMIT_COUNT 1)
    string(STRIP "${DUO_PINNED_HASH}" DUO_PINNED_HASH)
    if (DUO_PINNED_HASH MATCHES "^[0-9a-f]+$")
        message(STATUS "Duo: pinning shader cache version ${DUO_PINNED_HASH} (computed ${SHADER_CACHE_VERSION})")
        set(SHADER_CACHE_VERSION "${DUO_PINNED_HASH}")
    endif()
endif()
configure_file("${CMAKE_SOURCE_DIR}/src/common/scm_rev.cpp.in" "scm_rev.cpp" @ONLY)
