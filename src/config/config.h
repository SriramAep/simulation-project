
#pragma once
#include <string>

using namespace std;

struct AppConfig {

    int nth_failure         {5};
    double failure_rate     {0.8};
    int latency_ms          {2000};
    int cpu_multiplier      {100000};
    string log_level        {"debug"};      // Possible values: "debug", "warn", "info", "error"
    string failure_mode     {"REALISTIC"};  // Possible values: "REALISTIC", "DETERMINISTIC"
    string processor_mode   {"FUNCTIONAL"}; // Possible values: "FUNCTIONAL", "FAILURE", "DELAYED"

};

class ConfigLoader {

public:
    
    static void init(const std::string& path);
    static AppConfig load(const string& path);
    static const AppConfig& get();

private:
    static AppConfig& instance();
};