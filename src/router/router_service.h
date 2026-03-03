#include "router.grpc.pb.h"
#include "processor.grpc.pb.h"
#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>

using namespace simulation;
using namespace std;
using namespace grpc;

enum class State {
    INIT,
    FANOUT,
    WAITING,
    AGGREGATING,
    COMPLETED
};

class SubCall;
class RequestContext;
class RouterServiceImplementation;

class SubCall {
public:
    SubCall(RequestContext* parent);

    Status status;
    unique_ptr<ClientContext> context;
    ProcessorRequest request;
    ProcessorResponse response;

    unique_ptr<ClientAsyncResponseReader<ProcessorResponse>> rpc;

    int retry_count {0};
    bool is_retry {false};
    Alarm retry_alarm;
    chrono::steady_clock::time_point start_time;

    RequestContext* parent;
};

class RequestContext {
public:
    RequestContext(RouterServiceImplementation* service);

    void Proceed(bool ok);
    void RetrySubCall(SubCall* sc);
    void OnSubCallComplete(SubCall* sc, bool ok);
    
private:
    void StartFanout();
    void ScheduleRetry(SubCall* sc);
    void Aggregate();

    bool IsTransientFailure(const Status& status);
    bool DeadlineExceeded();

    RouterServiceImplementation* service;

    State state {State::INIT};

    int fanout_total {0};
    int completed {0};
    int failed {0};
    int retry_budget {2};

    chrono::steady_clock::time_point deadline;
    chrono::high_resolution_clock::time_point start_time;

    RouterRequest request;
    RouterResponse response;

    ServerContext server_ctx;
    ServerAsyncResponseWriter<RouterResponse> responder;

    vector<unique_ptr<SubCall>> subcalls;
};

class RouterServiceImplementation {
public:
    RouterServiceImplementation(const std::string& address,
                  const std::string& processor_address);

    ~RouterServiceImplementation();

    void Run();
    void Shutdown();

private:
    void HandleServerRpcs();
    void HandleClientRpcs();

    std::string address;

    RouterService::AsyncService service;

    unique_ptr<Server> server;
    unique_ptr<CompletionQueue> cq;
    unique_ptr<ServerCompletionQueue> server_cq;
    unique_ptr<ProcessorService::Stub> processor_stub;

    thread server_thread;
    thread client_thread;

    friend class RequestContext;
};
