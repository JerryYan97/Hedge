#include "HLogger.h"

namespace Hedge
{
    std::shared_ptr<spdlog::logger> HLogger::m_pCoreLogger;
    std::shared_ptr<spdlog::logger> HLogger::m_pClientLogger;

    HLogger::HLogger()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        m_pCoreLogger = spdlog::stdout_color_mt("Hedge Core");
        m_pCoreLogger->set_level(spdlog::level::trace);

        m_pClientLogger = spdlog::stdout_color_mt("Hedge Client");
        m_pClientLogger->set_level(spdlog::level::trace);
    }

    HLogger::~HLogger()
    {
    }

    std::shared_ptr<spdlog::logger>& HLogger::GetCoreLogger()
    {
        return m_pCoreLogger;
    }

    std::shared_ptr<spdlog::logger>& HLogger::GetClientLogger()
    {
        return m_pClientLogger;
    }
};