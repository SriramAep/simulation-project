#include <string>
#include "config.h"
#include <yaml-cpp/yaml.h>
#include "../observability/logging/logger.h"

using namespace std;

AppConfig& ConfigLoader::instance() {
    static AppConfig config;
    return config;
}

void ConfigLoader::init(const string& path) {
    instance() = ConfigLoader::load(path);
}

const AppConfig& ConfigLoader::get() {
    return instance();
}

AppConfig ConfigLoader::load(const string& path) {

    auto logger = logging::getLogger("config");
    logger->info("Loading Configuration from the file {}", path);

    AppConfig config;
    YAML::Node node = YAML::LoadFile(path);

    try {
        if (node["nthFailure"])
            config.nth_failure = node["nthFailure"].as<double>();
    
        if (node["failureRate"])
            config.failure_rate = node["failureRate"].as<double>();
    
        if (node["latencyMS"])
            config.latency_ms = node["latencyMS"].as<int>();
    
        if (node["cpuMultiplier"])
            config.cpu_multiplier = node["cpuMultiplier"].as<int>();
    
        if (node["logLevel"])
            config.log_level = node["logLevel"].as<string>();
    
        if (node["failureMode"])
            config.failure_mode = node["failureMode"].as<string>();
    
        if (node["processorMode"])
            config.processor_mode = node["processorMode"].as<string>();
    
        logger->info("Loaded Configuration successfully");
        return config;
    }
    catch (const YAML::Exception& e) {
        logger->error("YAML Error: {}", e.what());
        logger->error("Error parsing the configuration yaml. Default configuration will be used.");
        return config;
    }
}