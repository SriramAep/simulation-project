#pragma once
#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

namespace logging {

    void init(const string& log_level);
    void set_level(const string& log_level);
    shared_ptr<spdlog::logger> getLogger(const string& component);

}