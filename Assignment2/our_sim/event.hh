#ifndef EVENT_HH
#define EVENT_HH

#include "common.hh"

enum class EventType {
    ARRIVAL, // Request arrives into request buffer and waits for a free thread to assign
    DEPARTURE, // Request departs from the system after processing is done and thread is freed
    TIMEOUT, 
    THREAD_ARRIVAL, // Thread arrives into thread buffer
    THREAD_PROCESS // Thread enters server and starts processing a request
};

class Event {
public:
    EventType type;
    double timestamp; // Time at which the event occurs
    Event(double timestamp, EventType type, Request* request = nullptr, Thread* thread = nullptr);
    Request* request; // Associated request for arrival and departure events
    Thread* thread; // Associated thread for thread arrival and thread process events
};

struct EventComparator {
    bool operator()(Event* a, Event* b) {
        if (a->timestamp == b->timestamp) {
            // Departure should be prioritized over arrival if they occur at the same time to free up threads for waiting requests
            return a->type == EventType::DEPARTURE;
        }
        return a->timestamp > b->timestamp; // Min-heap based on timestamp
    }
};

#endif