#include "common.hh"

Sim::Sim(Config config) : receiver(config.num_threads), worker(config.total_cores) {
    // Initialize distributions based on config
    // Initialize event queue with the first arrival event
    float initial_arrival_time = 0.0; // This can be generated based on the arrival time distribution
    event_queue.push(new Event(initial_arrival_time, EventType::ARRIVAL));
}