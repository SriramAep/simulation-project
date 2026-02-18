// to avoid 
#pragma once

#include <memory>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>

using namespace prometheus;
using namespace std;

class Metrics {
    
public:
    static Metrics& instance();

private:
    Metrics();

    shared_ptr<Registry> registry_;
    unique_ptr<Exposer> exposer_;

public:
    // Counters metrics
    Counter&    total_requests; 
    Counter&    total_failures;
    
    // Gauge metrics
    Gauge&      inflight_requests;

    // Histogram metrics
    Histogram&  request_latency;


};