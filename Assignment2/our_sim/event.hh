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
    float timestamp; // Time at which the event occurs
    Event(float timestamp, EventType type);
};

struct EventComparator {
    bool operator()(Event* a, Event* b) {
        return a->timestamp > b->timestamp; // Min-heap based on timestamp
    }
};

#endif