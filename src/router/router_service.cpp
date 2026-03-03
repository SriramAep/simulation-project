#include <bits/stdc++.h>
#include "../observability/logging/logger.h"
#include "../observability/metrics/metrics.h"
#include "router_service.h"
#include "../config/config.h"

using namespace simulation;
using namespace std;
using namespace grpc;

// ##################################################
// Global functions/variables
// ##################################################

atomic<bool> shutdown_requested = false;

void signal_handler(int) {
    shutdown_requested = true;
}

// ##################################################
// RouterServiceImplementation class methods
// ##################################################
RouterServiceImplementation::RouterServiceImplementation(const std::string& address, const std::string& processor_address) : address(address) {

    auto logger = logging::getLogger("router");
    ServerBuilder builder;

    builder.AddListeningPort(address, InsecureServerCredentials());
    builder.RegisterService(&service);
    
    server_cq = builder.AddCompletionQueue();
    server = builder.BuildAndStart();

    processor_stub = ProcessorService::NewStub(CreateChannel(processor_address, InsecureChannelCredentials()));
    cq = make_unique<CompletionQueue>();
    logger->info("Router listening on {}", address);
}

RouterServiceImplementation::~RouterServiceImplementation() {
    Shutdown();
}

void RouterServiceImplementation::Run() {
    // Inital first RequestContext object to start accepting requests
    new RequestContext(this);

    server_thread = thread(&RouterServiceImplementation::HandleServerRpcs, this);
    client_thread = thread(&RouterServiceImplementation::HandleClientRpcs, this);
}

void RouterServiceImplementation::HandleServerRpcs() {
    void* tag;
    bool ok;

    while(server_cq->Next(&tag, &ok)) {
        static_cast<RequestContext*>(tag)->Proceed(ok);
    } 
}

void RouterServiceImplementation::HandleClientRpcs() {
    void* tag;
    bool ok;

    while(cq->Next(&tag, &ok)) {
        SubCall* sc = static_cast<SubCall*> (tag);
        if (sc->is_retry) {
            sc->is_retry = false;
            sc->parent->RetrySubCall(sc);
        } else {
            sc->parent->OnSubCallComplete(sc, ok);
        }
    }
}

void RouterServiceImplementation::Shutdown() {
    if (cq) cq->Shutdown();
    if (server) server->Shutdown();
    if (server_cq) server_cq->Shutdown();

    if (server_thread.joinable()) server_thread.join();
    if (client_thread.joinable()) client_thread.join();
}

// ##################################################
// SubCall class methods
// ##################################################
SubCall::SubCall(RequestContext* parent) : parent(parent) { }



// ##################################################
// RequestContext class methods
// ##################################################

RequestContext::RequestContext(RouterServiceImplementation* service) : service(service), responder(&server_ctx) {
    service->service.RequestRouteRequest(
        &server_ctx,
        &request,
        &responder,
        service->server_cq.get(),
        service->server_cq.get(),
        this);
}

void RequestContext::Proceed(bool ok) {
    if (!ok) {
        delete this;
        return;
    }

    if (state == State::INIT) {
        // Creating new RequestContext object to accept the next request
        new RequestContext(service);

        state = State::FANOUT;
        start_time = chrono::high_resolution_clock::now();
        StartFanout();
    } else if (state == State::COMPLETED) {
        delete this;
    }
}

void RequestContext::StartFanout() {

    fanout_total = request.fanout();
    auto now = chrono::steady_clock::now();
    deadline = now + chrono::milliseconds(request.timeout_ms());

    for (int i = 0; i < fanout_total; i++) {
        auto sc = make_unique<SubCall>(this);
        sc->request.set_workload(request.workload());
        sc->request.set_request_id(request.request_id());
        sc->start_time = chrono::steady_clock::now();
        sc->context = make_unique<ClientContext>();

        // Calculating remaining time left from the total deadline time, for every request
        auto remaining = deadline - sc->start_time;
        sc->context->set_deadline(chrono::system_clock::now() + remaining);
        sc->rpc = service->processor_stub->AsyncProcessor(sc->context.get(), sc->request, service->cq.get());
        sc->rpc->Finish(&sc->response, &sc->status, sc.get());

        subcalls.push_back(move(sc));
    }

    state = State::WAITING;
}

void RequestContext::OnSubCallComplete(SubCall* sc, bool ok) {
    if (!ok) {
        failed++;
    } else if (sc->status.ok()) {
        completed++;
    } else if (IsTransientFailure(sc->status) && sc->retry_count < retry_budget && !DeadlineExceeded()) {
        ScheduleRetry(sc);
        return;
    } else {
        failed++;
    }

    if (completed + failed == fanout_total) {
        state = State::AGGREGATING;
        Aggregate();
    }
}

void RequestContext::ScheduleRetry(SubCall* sc) {
    int backoff_ms = 10 * (1 << sc->retry_count);
    auto timer = chrono::system_clock::now() + chrono::milliseconds(backoff_ms);

    sc->is_retry = true;
    sc->retry_alarm.Set(service->cq.get(), timer, sc);
}

void RequestContext::RetrySubCall(SubCall* sc) {
    sc->retry_count++;
    
    if (DeadlineExceeded()) {
        failed++;
        return;
    }
    
    sc->context = make_unique<ClientContext>();
    auto remaining = deadline - chrono::steady_clock::now();
    sc->context->set_deadline(chrono::system_clock::now() + remaining);

    sc->rpc = service->processor_stub->AsyncProcessor(sc->context.get(), sc->request, service->cq.get());
    sc->rpc->Finish(&sc->response, &sc->status, sc);
}

void RequestContext::Aggregate() {

    auto logger = logging::getLogger("router");
    logger->info("Request results - total: {}, success: {}, failure: {}", fanout_total, completed, failed);

    response.set_success(completed);
    response.set_failure(failed);
    auto duration = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time);
    response.set_latency_ms(duration.count());

    state = State::COMPLETED;
    responder.Finish(response, Status::OK, this);
}

bool RequestContext::IsTransientFailure(const Status& status) {
    return status.error_code() == StatusCode::UNAVAILABLE || 
           status.error_code() == StatusCode::DEADLINE_EXCEEDED;
}

bool RequestContext::DeadlineExceeded() {
    return chrono::steady_clock::now() >= deadline;
}

int main() {

    // Logging initialisation
    logging::init(ConfigLoader::get().log_level);

    // Configuration init
    ConfigLoader::init("src/config/config.yaml");
    logging::set_level(ConfigLoader::get().log_level);

    auto logger = logging::getLogger("router");
    logger->info("Router service starting");

    // auto& metrics = Metrics::instance();
    string router_address = "0.0.0.0:50052";
    string processor_address = "0.0.0.0:50051";
    RouterServiceImplementation router(router_address, processor_address);
    router.Run();

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (!shutdown_requested) {
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    router.Shutdown();

    return 0;
}