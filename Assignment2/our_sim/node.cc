#include "node.hh"

void ServerNode::process(Event* current_event, Sim* sim) {
    switch (current_event->type) {
        case EventType::ARRIVAL: {
            if (receiver.thread_pool.has_idle_thread()) {
                Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                idle_thread->current_request = current_event->request;
                // Schedule Thread Arrive event for this thread
                Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, current_event->request, idle_thread, nullptr, this);
                sim->event_queue.push(thread_arrival_event);
            } else {
                // If request buffer is full, the request is dropped (not added to the queue)
                if ((int)receiver.request_queue.size() < receiver.receiver_buffer_size) {
                    receiver.request_queue.push(current_event->request);
                } else {
                    // Request is dropped, we can log this if needed
                }
            }
            break;
        }
        case EventType::THREAD_ARRIVAL: {
            if (worker.has_free_core()) {
                Core* assigned_core = worker.find_free_core();
                if (!assigned_core->busy && assigned_core->thread_buffer.empty()) {
                    // Schedule a thread process event for this thread to start processing
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, assigned_core, this);
                    sim->event_queue.push(thread_process_event);
                } else {
                    assigned_core->thread_buffer.push(current_event->thread);
                }
            } else {
                // No free core available, try to add the thread to the worker's thread queue
                if ((int)worker.thread_queue.size() < worker.thread_buffer_size) {
                    worker.thread_queue.push(current_event->thread);
                } else {
                    // Thread is dropped, we can log this if needed
                    // Free the thread since it cannot be processed
                    current_event->thread->current_request = nullptr;
                }
            }                
            break;
        }
        case EventType::THREAD_PROCESS: {
            double service_time_remaining = current_event->request->remaining_service_time;
            if (service_time_remaining <= current_event->core->core_context_switch_time) {
                // Schedule departure event
                Event* departure_event = new Event(current_event->timestamp + service_time_remaining, EventType::DEPARTURE, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(departure_event);
                current_event->core->busy = true;
                worker.busy_cores++;
                current_event->request->remaining_service_time = 0;
            } else {
                // Schedule context switch event
                Event* context_switch_event = new Event(current_event->timestamp + current_event->core->core_context_switch_time, EventType::CONTEXT_SWITCH, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(context_switch_event);
                current_event->core->busy = true;
                worker.busy_cores++;
                current_event->request->remaining_service_time -= current_event->core->core_context_switch_time;
            }
            break;
        }
        case EventType::CONTEXT_SWITCH: {
            if (!current_event->core->thread_buffer.empty()) {
                Thread* next_thread = current_event->core->thread_buffer.front();
                current_event->core->thread_buffer.pop();
                Event* thread_process_event = new Event(current_event->timestamp + current_event->core->core_context_switch_overhead, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
                current_event->core->thread_buffer.push(current_event->thread); // Put the current thread back into the core's thread buffer
            } else {
                // No waiting threads in the core's buffer, schedule a thread process event for the current thread to continue processing immediately
                Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
            }
            break;                
        }
        case EventType::DEPARTURE: {
            current_event->request->departure_time = current_event->timestamp;
            current_event->core->busy = false;
            worker.busy_cores--;
            current_event->thread->current_request = nullptr; // Free the thread
            
            Event* next_event = new Event(current_event->timestamp, EventType::ARRIVAL, current_event->request, nullptr, nullptr, this->get_next(current_event->request));
            sim->event_queue.push(next_event);
        
            if (!current_event->core->thread_buffer.empty()) {
                Thread* next_thread = current_event->core->thread_buffer.front();
                current_event->core->thread_buffer.pop();
                Event* thread_process_event = new Event(current_event->timestamp + current_event->core->core_context_switch_overhead, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
                worker.busy_cores++;
                current_event->core->busy = true;
            }
            if (!worker.thread_queue.empty()) {
                Thread* next_thread = worker.thread_queue.front();
                worker.thread_queue.pop();
                current_event->core->thread_buffer.push(next_thread);
                if (!current_event->core->busy) {
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                    sim->event_queue.push(thread_process_event);
                    worker.busy_cores++;
                    current_event->core->busy = true;
                }
            } 
            if (!receiver.request_queue.empty()) {
                Request* next_request = receiver.request_queue.front();
                receiver.request_queue.pop();
                Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                idle_thread->current_request = next_request;
                Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, next_request, idle_thread, nullptr, this);
                sim->event_queue.push(thread_arrival_event);
            }
            break;
        }
        default:
            break;
    }
}

void JunctionNode::process(Event* event, Sim* sim) {
    Request *req = event->request;
    Node* next_node = get_next(req);
    Event* next_event = new Event(event->timestamp, EventType::ARRIVAL, req, nullptr, nullptr, next_node);
    sim->event_queue.push(next_event);
}

void ClientNode::process(Event* event, Sim* sim) {
    double think_time = sim->think_time_dist->sample();
    Node* next_node = get_next(event->request);
    Event* next_event = new Event(event->timestamp + think_time, EventType::ARRIVAL, event->request, nullptr, nullptr, next_node);
    sim->event_queue.push(next_event);
}