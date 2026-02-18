
#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>
#include "processor.grpc.pb.h"
#include <chrono>
#include <thread>
#include "processor_service.h"
#include "../observability/metrics/metrics.h"
#include "../observability/logging/logger.h"

using namespace simulation;
using namespace grpc;
using namespace std;
using namespace prometheus;

// Global Variables (temporary configuration params for now)
string PROCESSOR_MODE = "FUNCTIONAL";
int LATENCY_MS = 2000;
int FAILURE_RATE = 0.1;
int CPU_MULTIPLIER = 100000;

class ProcessorExecution {

public:
    bool execute (int workload) {

        // Could add an enum for this FAILURE_MODE and check accordingly
        if (PROCESSOR_MODE == "FAILURE" || probable_failure(FAILURE_RATE)) {
            return false;
        }

        if (PROCESSOR_MODE == "DELAYED") {
            // latency_ms - a global variable loaded from configuration
            latency(LATENCY_MS);
        }

        volatile int result = 0;

        for (int i = 0; i < workload*CPU_MULTIPLIER; i++) {
            result += i;
        }

        return true;
    }

    void latency (int latency_ms) {
        // sleep of latency_ms
        this_thread::sleep_for(chrono::milliseconds(latency_ms));
    }

    bool probable_failure (int failure_rate) {
        // some code to calculate probability 
        // and return true/false

        // but returning false for now.
        return false;
    }

};

class ProcessorServiceImplementation : public ProcessorService::Service {

private:
    ProcessorExecution executor;

public:
    Status Processor (ServerContext* context, const ProcessorRequest* request, ProcessorResponse* response) {

        auto logger = logging::getLogger("processor");
        logger->debug("Request received workload={}", request->workload());

        string result_str = "success";
        auto& metrics = Metrics::instance();

        metrics.inflight_requests.Increment();
        metrics.total_requests.Increment();

        auto start_time = chrono::high_resolution_clock::now();
        bool result = executor.execute(request->workload());
        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>( end_time - start_time );
        auto hist_duration = chrono::duration<double>( end_time - start_time ).count();
        metrics.request_latency.Observe(hist_duration);

        if ( !result ) {
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
};

namespace processor {
    void RunServer () {

        auto logger = logging::getLogger("processor");

        string server_address = "0.0.0.0:50051";
        ProcessorServiceImplementation service;
        ServerBuilder builder;
    
        builder.AddListeningPort(server_address, InsecureServerCredentials());
        builder.RegisterService(&service);
        unique_ptr<Server> server(builder.BuildAndStart());

        logger->info("Server listening on {}", server_address);
    
        server->Wait();
    }
}