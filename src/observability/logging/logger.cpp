#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <cstdlib>
#include <bits/stdc++.h>
#include <mutex>
#include <stdexcept>

using namespace std;

namespace logging {

    shared_ptr<spdlog::sinks::sink> shared_sink;
    unordered_map<string, shared_ptr<spdlog::logger>> logger_registry;
    mutex logger_registry_mutex;

    void init() {

        shared_sink = make_shared<spdlog::sinks::stdout_color_sink_mt>();

        const char* level_env = getenv("LOG_LEVEL");

        spdlog::level::level_enum level = spdlog::level::info;
        
        if ( level_env ) {
            try {
                level = spdlog::level::from_str(level_env);
            } catch (...) {
                level = spdlog::level::info;
            }
        }

        spdlog::set_level(level);
        spdlog::set_pattern(R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%eZ","level":"%l","component":"%n","message":"%v"})");
    }

    shared_ptr<spdlog::logger> getLogger(const string& component) {
        
        lock_guard<mutex> lock (logger_registry_mutex);

        if ( logger_registry.count(component) == 0 ) {
            auto logger = make_shared<spdlog::logger>(component, shared_sink);
            
            logger->set_level(spdlog::get_level());
            spdlog::register_logger(logger);
            
            logger_registry[component] = logger;
        }

        return logger_registry[component];
    }

}