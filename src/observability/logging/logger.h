#pragma once
#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

namespace logging {

    void init();
    shared_ptr<spdlog::logger> getLogger(const string& component);

}