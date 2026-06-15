#include "PCH.h"

#include "Logger.h"

namespace DialogueHUDFade::Logger
{
    bool Initialize()
    {
        try {
            auto logPath = SKSE::log::log_directory();
            if (!logPath) {
                return false;
            }

            *logPath /= "DialogueHUDFade.log";

            std::vector<spdlog::sink_ptr> sinks;
            sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath->string(), true));

            if (::IsDebuggerPresent()) {
                sinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
            }

            auto logger = std::make_shared<spdlog::logger>("DialogueHUDFade", sinks.begin(), sinks.end());
            logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            logger->set_level(spdlog::level::info);
            logger->flush_on(spdlog::level::warn);

            spdlog::set_default_logger(std::move(logger));
            spdlog::info("DialogueHUDFade logging initialized");
            return true;
        } catch (const std::exception& ex) {
            if (::IsDebuggerPresent()) {
                const std::string message = std::string("DialogueHUDFade logger initialization failed: ") + ex.what() + '\n';
                ::OutputDebugStringA(message.c_str());
            }
        } catch (...) {
            if (::IsDebuggerPresent()) {
                ::OutputDebugStringA("DialogueHUDFade logger initialization failed with an unknown error\n");
            }
        }

        return false;
    }

    void SetDebugLogging(bool a_enabled)
    {
        spdlog::set_level(a_enabled ? spdlog::level::debug : spdlog::level::info);
        spdlog::flush_on(a_enabled ? spdlog::level::debug : spdlog::level::warn);
    }
}
