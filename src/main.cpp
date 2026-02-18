#include "observability/metrics/metrics.h"
#include "processor/processor_service.h"
#include "observability/logging/logger.h"

int main() {
    
    logging::init();
    
    auto main_logger = logging::getLogger("processor");
    main_logger->info("Processor service starting");
    
    auto& metrics = Metrics::instance();
    processor::RunServer();

    return 0;
}