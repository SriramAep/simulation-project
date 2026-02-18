#include "metrics.h"
#include <vector>

using namespace std;
using namespace prometheus;

Metrics&  Metrics::instance() {
    static Metrics instance;
    return instance;
}

Metrics::Metrics()
    : registry_(make_shared<Registry>()),
      exposer_(make_unique<Exposer>("0.0.0.0:8081")),
      // Counter Metrics
      total_requests (
          BuildCounter()
          .Name("processor_requests_total")
          .Help("Total number of requests received by the processor")
          .Register(*registry_)
          .Add({})
      ),
  
      total_failures (
          BuildCounter()
          .Name("processor_failures_total")
          .Help("Total number of requests failed to process")
          .Register(*registry_)
          .Add({})
      ),
  
      // Gauge metrics
      inflight_requests (
          BuildGauge()
          .Name("processor_inflight_requests")
          .Help("Total number of requests currently in processing")
          .Register(*registry_)
          .Add({})
      ),
  
      // Histogram metrics
      request_latency (
          BuildHistogram()
          .Name("processor_latency_histogram")
          .Help("Histogram of processor request latency")
          .Register(*registry_)
          .Add({}, vector<double>{0.01, 0.05, 0.1, 0.5, 1.0})
      )
{
    exposer_->RegisterCollectable(registry_);
}