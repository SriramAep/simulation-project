
#include <bits/stdc++.h>
#include "processor.grpc.pb.h"
#include <chrono>
#include <thread>
#include "processor_service.h"
#include "../observability/metrics/metrics.h"
#include "../observability/logging/logger.h"
#include "../config/config.h"

using namespace simulation;
using namespace grpc;
using namespace std;

Status ProcessorServiceImplementation::Processor (ServerContext* context, const ProcessorRequest* request, ProcessorResponse* response) {

    string result_str = "success";
    auto logger = logging::getLogger("processor");
    logger->debug("Request received workload={}", request->workload());

    auto& metrics = Metrics::instance();
    metrics.total_requests.Increment();

    if (executor.introduce_failure()) {
        metrics.total_failures.Increment();
        logger->warn("Failure injected for request_id {}, failure mode - {}", request->request_id(), ConfigLoader::get().failure_mode);
        return Status(StatusCode::UNAVAILABLE, "Injected failure");
    }

    metrics.inflight_requests.Increment();

    auto start_time = chrono::high_resolution_clock::now();
    bool result = executor.execute(request->workload());
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>( end_time - start_time );
    auto hist_duration = chrono::duration<double>( end_time - start_time ).count();
    metrics.request_latency.Observe(hist_duration);

    if (!result) {
        metrics.total_failures.Increment();
        result_str = "failure";
    }

    logger->info("request_id={} workload={} latency_ms={} status={}",
    request->request_id(), request->workload(), duration.count(), result_str);

    response->set_success(result);
    response->set_latency_ms(duration.count());
    
    metrics.inflight_requests.Decrement();

    return Status::OK;
}

bool ProcessorExecution::execute (int workload) {

    // Could add an enum for this FAILURE_MODE and check accordingly
    if (ConfigLoader::get().failure_mode == "FAILURE") {
        return false;
    }

    if (ConfigLoader::get().processor_mode == "DELAYED") {
        // latency_ms - a global variable loaded from configuration
        latency(ConfigLoader::get().latency_ms);
    }

    volatile int result = 0;

    for (int i = 0; i < workload*ConfigLoader::get().cpu_multiplier; i++) {
        result += i;
    }

    return true;
}

void ProcessorExecution::latency (int latency_ms) {
    // sleep of latency_ms
    this_thread::sleep_for(chrono::milliseconds(latency_ms));
}

bool ProcessorExecution::introduce_failure () {

    if (ConfigLoader::get().failure_mode == "REALISTIC") {
        static thread_local mt19937 generator(random_device{}());
        uniform_real_distribution<double> distribution(0.0, 1.0);

        return distribution(generator) < ConfigLoader::get().failure_rate;
    } else if (ConfigLoader::get().failure_mode == "DETERMINISTIC") {
        static atomic<int> counter = 0;
        int current = ++counter;
        return !(current % ConfigLoader::get().nth_failure);
    }

    return false;
}

void ProcessorExecution::runServer (const string& server_address) {

    auto logger = logging::getLogger("processor");

    ProcessorServiceImplementation service;
    ServerBuilder builder;

    builder.AddListeningPort(server_address, InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server (builder.BuildAndStart());

    logger->info("Processor listening on {}", server_address);

    server->Wait();
}

int main() {

    // Logging initialisation
    logging::init(ConfigLoader::get().log_level);

    // Configuration init
    ConfigLoader::init("src/config/config.yaml");
    logging::set_level(ConfigLoader::get().log_level);

    auto logger = logging::getLogger("processor");
    logger->info("Processor service starting");
    
    auto& metrics = Metrics::instance();
    ProcessorExecution processor;
    string server_address = "0.0.0.0:50051";
    processor.runServer(server_address);

    return 0;
}