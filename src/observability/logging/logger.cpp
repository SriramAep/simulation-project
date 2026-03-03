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

    void init(const string& log_level) {

        shared_sink = make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        set_level(log_level);
        spdlog::set_pattern(R"({"timestamp":"%Y-%m-%dT%H:%M:%S.%eZ","level":"%l","component":"%n","message":"%v"})");
    }

    void set_level(const std::string& log_level) {

        spdlog::level::level_enum level;

        try {
            level = spdlog::level::from_str(log_level);
        } catch (...) {
            level = spdlog::level::info;
        }

        spdlog::set_level(level);
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