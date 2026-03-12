#include "node.hh"

void ServerNode::process(Event* current_event, NetworkSim* sim) {
    std::ofstream output_file("network_log.txt", std::ios_base::app); // Open the file in append mode
    switch (current_event->type) {
        case EventType::ARRIVAL: {
            output_file << "[Time " << current_event->timestamp << "] Request ID: " << current_event->request->id << " arrived at Server Node ID: " << this->id << std::endl;
            current_event->request->service_time = this->service_time_dist->sample();
            current_event->request->remaining_service_time = current_event->request->service_time;
            if (receiver.thread_pool.has_idle_thread()) {
                Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                idle_thread->current_request = current_event->request;
                // Schedule Thread Arrive event for this thread
                Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, current_event->request, idle_thread, nullptr, this);
                sim->event_queue.push(thread_arrival_event);
                output_file << "Assigned Request ID: " << current_event->request->id << " to idle Thread ID: " << idle_thread->id << std::endl;
            } else {
                // If request buffer is full, the request is dropped (not added to the queue)
                if ((int)receiver.request_queue.size() < receiver.receiver_buffer_size) {
                    receiver.request_queue.push(current_event->request);
                    output_file << "No idle threads. Added Request ID: " << current_event->request->id << " to the receiver queue at Server Node ID: " << this->id << std::endl;
                } else {
                    current_event->request->is_dropped = true;
                    // Request is dropped, we can log this if needed
                    output_file << "[Time " << current_event->timestamp << "] Request ID: " << current_event->request->id << " dropped at Server Node ID: " << this->id << std::endl;
                }
            }
            break;
        }
        case EventType::THREAD_ARRIVAL: {
            output_file << "[Time " << current_event->timestamp << "] Thread ID: " << current_event->thread->id << " arrived at worker of Server Node ID: " << this->id << std::endl;
            if (worker.has_free_core()) {
                Core* assigned_core = worker.find_free_core();
                if (!assigned_core->busy && assigned_core->thread_buffer.empty()) {
                    // Schedule a thread process event for this thread to start processing
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, assigned_core, this);
                    sim->event_queue.push(thread_process_event);
                    assigned_core->busy = true;
                    worker.busy_cores++;
                    output_file << "Assigned Thread ID: " << current_event->thread->id << " to free Core ID: " << assigned_core->id << " for processing at Server Node ID: " << this->id << std::endl;
                } else {
                    assigned_core->thread_buffer.push(current_event->thread);
                    output_file << "No free cores. Added Thread ID: " << current_event->thread->id << " to the thread buffer of Core ID: " << assigned_core->id << " at Server Node ID: " << this->id << std::endl;
                }
            } else {
                // No free core available, try to add the thread to the worker's thread queue
                if ((int)worker.thread_queue.size() < worker.thread_buffer_size) {
                    worker.thread_queue.push(current_event->thread);
                    output_file << "No free cores. Added Thread ID: " << current_event->thread->id << " to the worker thread queue at Server Node ID: " << this->id << std::endl;
                } else {
                    // Thread is dropped, we can log this if needed
                    current_event->request->is_dropped = true;
                    output_file << "No free cores and thread buffer full. Thread ID: " << current_event->thread->id << " dropped at Server Node ID: " << this->id << std::endl;
                    // Free the thread since it cannot be processed
                    current_event->thread->current_request = nullptr;
                }
            }                
            break;
        }
        case EventType::THREAD_PROCESS: {
            output_file << "[Time " << current_event->timestamp << "] Processing Thread ID: " << current_event->thread->id << " on Core ID: " << current_event->core->id << " at Server Node ID: " << this->id << std::endl;
            double service_time_remaining = current_event->request->remaining_service_time;
            if (service_time_remaining <= current_event->core->core_context_switch_time) {
                // Schedule departure event
                Event* departure_event = new Event(current_event->timestamp + service_time_remaining, EventType::DEPARTURE, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(departure_event);
                current_event->core->busy = true;
                worker.busy_cores++;
                current_event->request->remaining_service_time = 0;
                output_file << "Scheduled DEPARTURE event for Request ID: " << current_event->request->id << " at time " << current_event->timestamp + service_time_remaining << std::endl;
            } else {
                // Schedule context switch event
                Event* context_switch_event = new Event(current_event->timestamp + current_event->core->core_context_switch_time, EventType::CONTEXT_SWITCH, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(context_switch_event);
                current_event->core->busy = true;
                worker.busy_cores++;
                current_event->request->remaining_service_time -= current_event->core->core_context_switch_time;
                output_file << "Scheduled CONTEXT_SWITCH event for Thread ID: " << current_event->thread->id << " at time " << current_event->timestamp + current_event->core->core_context_switch_time << std::endl;
            }
            break;
        }
        case EventType::CONTEXT_SWITCH: {
            output_file << "[Time " << current_event->timestamp << "] Context switch for Thread ID: " << current_event->thread->id << " on Core ID: " << current_event->core->id << " at Server Node ID: " << this->id << std::endl;
            if (!current_event->core->thread_buffer.empty()) {
                Thread* next_thread = current_event->core->thread_buffer.front();
                current_event->core->thread_buffer.pop();
                Event* thread_process_event = new Event(current_event->timestamp + current_event->core->core_context_switch_overhead, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
                current_event->core->thread_buffer.push(current_event->thread); // Put the current thread back into the core's thread buffer
                output_file << "Context switch: Moved Thread ID: " << current_event->thread->id << " to the thread buffer of Core ID: " << current_event->core->id << " and scheduled Thread ID: " << next_thread->id << " for processing at Server Node ID: " << this->id << std::endl;
            } else {
                // No waiting threads in the core's buffer, schedule a thread process event for the current thread to continue processing immediately
                Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
                output_file << "Context switch: No waiting threads. Rescheduled Thread ID: " << current_event->thread->id << " for processing at Server Node ID: " << this->id << std::endl;
            }
            break;                
        }
        case EventType::DEPARTURE: {
            output_file << "[Time " << current_event->timestamp << "] Departure of Request ID: " << current_event->request->id << " from Server Node ID: " << this->id << std::endl;
            current_event->core->busy = false;
            worker.busy_cores--;
            current_event->thread->current_request = nullptr; // Free the thread
            
            Node* next_node = this->get_next();
            Event* next_event = new Event(current_event->timestamp, EventType::ARRIVAL, current_event->request, nullptr, nullptr, next_node);
            sim->event_queue.push(next_event);
            output_file << "Scheduled ARRIVAL event for Request ID: " << current_event->request->id << " at time " << current_event->timestamp << "to Node ID: " << next_node->id << std::endl;
        
            if (!current_event->core->thread_buffer.empty()) {
                Thread* next_thread = current_event->core->thread_buffer.front();
                current_event->core->thread_buffer.pop();
                Event* thread_process_event = new Event(current_event->timestamp + current_event->core->core_context_switch_overhead, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                sim->event_queue.push(thread_process_event);
                worker.busy_cores++;
                current_event->core->busy = true;
                output_file << "After departure, scheduled Thread ID: " << next_thread->id << " for processing on Core ID: " << current_event->core->id << " at Server Node ID: " << this->id << std::endl;
            }
            if (!worker.thread_queue.empty()) {
                Thread* next_thread = worker.thread_queue.front();
                worker.thread_queue.pop();
                current_event->core->thread_buffer.push(next_thread);
                output_file << "Departure: Moved Thread ID: " << current_event->thread->id << " to the thread buffer of Core ID: " << current_event->core->id << " at Server Node ID: " << this->id << std::endl;
                if (!current_event->core->busy) {
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core, this);
                    sim->event_queue.push(thread_process_event);
                    worker.busy_cores++;
                    current_event->core->busy = true;
                    output_file << "Departure: Scheduled Thread ID: " << next_thread->id << " for processing on Core ID: " << current_event->core->id << " at Server Node ID: " << this->id << std::endl;
                }
            } 
            if (!receiver.request_queue.empty()) {
                Request* next_request = receiver.request_queue.front();
                receiver.request_queue.pop();
                Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                idle_thread->current_request = next_request;
                Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, next_request, idle_thread, nullptr, this);
                sim->event_queue.push(thread_arrival_event);
                output_file << "After departure, assigned Request ID: " << next_request->id << " to idle Thread ID: " << idle_thread->id << " and scheduled THREAD_ARRIVAL event at Server Node ID: " << this->id << std::endl;
            }
            break;
        }
        default:
            break;
    }
}

void ClientNode::process(Event* event, NetworkSim* sim) {
    std::ofstream output_file("network_log.txt", std::ios_base::app); // Open the file in append mode
    
    if (event->type == EventType::TIMEOUT) {
        if (event->request->departure_time == -1) {
            event->request->timed_out = true;
            Request* new_request = new Request(event->request->user_id, event->timestamp, 0); // Service time will be assigned when the request arrives at the server
            sim->all_requests.push_back(new_request);
            Node* next_node = this->get_next();
            Event* arrival_event = new Event(event->timestamp, EventType::ARRIVAL, new_request, nullptr, nullptr, next_node);
            sim->event_queue.push(arrival_event);
            output_file << "[Time " << event->timestamp << "] TIMEOUT: User ID: " << event->request->user_id << " at Client Node ID: " << this->id << " timed out. Scheduled new ARRIVAL event for User ID: " << event->request->user_id << " at time " << event->timestamp << " to Node ID: " << next_node->id << std::endl;
        }
        return;
    }
    
    event->request->departure_time = event->timestamp; // Mark the departure time for statistics
    output_file << "[Time " << event->timestamp << "] User ID: " << event->request->user_id << " at Client Node ID: " << this->id << " is thinking." << std::endl;
    double think_time = this->think_time->sample();
    Node* next_node = get_next();
    Request* new_request = new Request(event->request->user_id, event->timestamp + think_time, 0); // Service time will be assigned when the request arrives at the server
    Event* next_event = new Event(event->timestamp + think_time, EventType::ARRIVAL, new_request, nullptr, nullptr, next_node);
    sim->event_queue.push(next_event);
    sim->all_requests.push_back(new_request);
    output_file << "Scheduled ARRIVAL event for User ID: " << event->request->user_id << " at time " << event->timestamp + think_time << " to Node ID: " << next_node->id << std::endl;

    Event* timeout_event = new Event(event->timestamp + think_time + min_timeout + this->timeout_dist->sample(), EventType::TIMEOUT, new_request, nullptr, nullptr, this);
    sim->event_queue.push(timeout_event);
    output_file << "Scheduled TIMEOUT event for User ID: " << event->request->user_id << " at time " << event->timestamp + think_time + min_timeout + this->timeout_dist->sample() << " at Client Node ID: " << this->id << std::endl;
}