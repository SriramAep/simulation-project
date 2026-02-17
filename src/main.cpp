#include "observability/metrics.h"
#include "processor/processor_service.h"

int main() {
    auto& metrics = Metrics::instance();

    
    RunServer();

    return 0;
}