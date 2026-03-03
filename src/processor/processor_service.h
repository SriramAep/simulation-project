#include "processor.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using namespace simulation;
using namespace grpc;

class ProcessorExecution {
public:
    bool execute (int workload);
    void latency (int latency_ms);
    bool introduce_failure();

    void runServer (const string& server_address);
};

class ProcessorServiceImplementation : public ProcessorService::Service {
private:
    ProcessorExecution executor;
public:
    Status Processor (ServerContext* context, const ProcessorRequest* request, ProcessorResponse* response);
};