#pragma once
#include <vulkan/vulkan.h>
#include <iostream>

namespace Hedge
{
    constexpr uint32_t VertFloatNum = 3 + 3 + 2;

    std::string SelectFolderDialog();

    std::string SelectYmlDialog();

    std::string GetExePath();

    // Stirng manipulations
    std::string GetFileName(const std::string& pathName);

    std::string GetFileDir(const std::string& pathName);
}

// https://stackoverflow.com/a/36522355
namespace cexp
{

    // Small implementation of std::array, needed until constexpr
    // is added to the function 'reference operator[](size_type)'
    template <typename T, std::size_t N>
    struct array {
        T m_data[N];

        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = std::size_t;

        // This is NOT constexpr in std::array until C++17
        constexpr reference operator[](size_type i) noexcept {
            return m_data[i];
        }

        constexpr const_reference operator[](size_type i) const noexcept {
            return m_data[i];
        }

        constexpr size_type size() const noexcept {
            return N;
        }
    };

}

// Generates CRC-32 table, algorithm based from this link:
// http://www.hackersdelight.org/hdcodetxt/crc.c.txt
constexpr auto gen_crc32_table() {
    constexpr auto num_bytes = 256;
    constexpr auto num_iterations = 8;
    constexpr auto polynomial = 0xEDB88320;

    auto crc32_table = cexp::array<uint32_t, num_bytes>{};

    for (auto byte = 0u; byte < num_bytes; ++byte) {
        auto crc = byte;

        for (auto i = 0; i < num_iterations; ++i) {
            auto mask = -(crc & 1);
            crc = (crc >> 1) ^ (polynomial & mask);
        }

        crc32_table[byte] = crc;
    }

    return crc32_table;
}

// Stores CRC-32 table and softly validates it.
static constexpr auto crc32_table = gen_crc32_table();
static_assert(
    crc32_table.size() == 256 &&
    crc32_table[1] == 0x77073096 &&
    crc32_table[255] == 0x2D02EF8D,
    "gen_crc32_table generated unexpected result."
);

// Generates CRC-32 code from null-terminated, c-string,
// algorithm based from this link:
// http://www.hackersdelight.org/hdcodetxt/crc.c.txt 
constexpr auto crc32(const char* in) {
    auto crc = 0xFFFFFFFFu;

    for (auto i = 0u; auto c = in[i]; ++i) {
        crc = crc32_table[(crc ^ c) & 0xFF] ^ (crc >> 8);
    }

    return ~crc;
}

#ifndef NDEBUG

// Debug mode

#define STR(r)    \
	case r:       \
		return #r \

static const std::string to_string(VkResult result)
{
    switch (result) {
        STR(VK_NOT_READY);
        STR(VK_TIMEOUT);
        STR(VK_EVENT_SET);
        STR(VK_EVENT_RESET);
        STR(VK_INCOMPLETE);
        STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        STR(VK_ERROR_INITIALIZATION_FAILED);
        STR(VK_ERROR_DEVICE_LOST);
        STR(VK_ERROR_MEMORY_MAP_FAILED);
        STR(VK_ERROR_LAYER_NOT_PRESENT);
        STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        STR(VK_ERROR_FEATURE_NOT_PRESENT);
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        STR(VK_ERROR_TOO_MANY_OBJECTS);
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        STR(VK_ERROR_FRAGMENTED_POOL);
        STR(VK_ERROR_UNKNOWN);
        STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        STR(VK_ERROR_FRAGMENTATION);
        STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
        STR(VK_ERROR_SURFACE_LOST_KHR);
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(VK_SUBOPTIMAL_KHR);
        STR(VK_ERROR_OUT_OF_DATE_KHR);
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(VK_ERROR_VALIDATION_FAILED_EXT);
        STR(VK_ERROR_INVALID_SHADER_NV);
        STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
        STR(VK_ERROR_NOT_PERMITTED_EXT);
        STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
        STR(VK_THREAD_IDLE_KHR);
        STR(VK_THREAD_DONE_KHR);
        STR(VK_OPERATION_DEFERRED_KHR);
        STR(VK_OPERATION_NOT_DEFERRED_KHR);
        STR(VK_PIPELINE_COMPILE_REQUIRED_EXT);
    default:
        return "UNKNOWN_ERROR";
    }
}

#define VK_CHECK(res) if(res){std::cout << "Error at line:" << __LINE__ << " in " << __FILE__ << ", Error name:" << to_string(res) << ".\n"; exit(1);}
#else

// Release mode

#define VK_CHECK(res) res
#endif