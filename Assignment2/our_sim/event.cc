#include "common.hh"

Event::Event(double timestamp, EventType type, Request* request, Thread* thread, Core* core) {
    this->timestamp = timestamp;
    this->type = type;
    this->request = request;
    this->thread = thread;
    this->core = core;
}

std::string event_type_to_string(EventType type) {
    switch (type) {
        case EventType::ARRIVAL:
            return "ARRIVAL";
        case EventType::DEPARTURE:
            return "DEPARTURE";
        case EventType::TIMEOUT:
            return "TIMEOUT";
        case EventType::CONTEXT_SWITCH:
            return "CONTEXT_SWITCH";
        case EventType::THREAD_ARRIVAL:
            return "THREAD_ARRIVAL";
        case EventType::THREAD_PROCESS:
            return "THREAD_PROCESS";
        default:
            return "UNKNOWN";
    }
}