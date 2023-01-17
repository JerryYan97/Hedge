#pragma once

#include <memory>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Hedge
{
    class HLogger
    {
    public:
        HLogger();
        ~HLogger();

        std::shared_ptr<spdlog::logger>& GetCoreLogger();
        std::shared_ptr<spdlog::logger>& GetClientLogger();

        static std::shared_ptr<spdlog::logger> m_pCoreLogger;
        static std::shared_ptr<spdlog::logger> m_pClientLogger;
    };
};

#ifndef NDEBUG
// Debug mode
#define HDG_CORE_ERROR(...) ::Hedge::HLogger::m_pCoreLogger->error(__VA_ARGS__)
#define HDG_CORE_WARN(...)  ::Hedge::HLogger::m_pCoreLogger->warn(__VA_ARGS__)
#define HDG_CORE_INFO(...)  ::Hedge::HLogger::m_pCoreLogger->info(__VA_ARGS__)
#define HDG_CORE_TRACE(...) ::Hedge::HLogger::m_pCoreLogger->trace(__VA_ARGS__)

#define HDG_ERROR(...) ::Hedge::HLogger::m_pClientLogger->error(__VA_ARGS__)
#define HDG_WARN(...)  ::Hedge::HLogger::m_pClientLogger->warn(__VA_ARGS__)
#define HDG_INFO(...)  ::Hedge::HLogger::m_pClientLogger->info(__VA_ARGS__)
#define HDG_TRACE(...) ::Hedge::HLogger::m_pClientLogger->trace(__VA_ARGS__)
#else
// Release mode
#define HDG_CORE_ERROR(...)
#define HDG_CORE_WARN(...)
#define HDG_CORE_INFO(...)
#define HDG_CORE_TRACE(...)
#endif