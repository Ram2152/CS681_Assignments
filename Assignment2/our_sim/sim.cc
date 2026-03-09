#include "common.hh"

Sim::Sim(Config config) : num_users(config.num_users), timeout(config.timeout), max_time(config.max_time), receiver(config.num_threads, config.request_buffer_size), worker(config.total_cores, config.thread_buffer_size, config.core_buffer_size, config.core_context_switch_time, config.core_context_switch_overhead) {
    std::ifstream config_file(config.input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }

    std::string think_dist, service_dist;
    int num_of_runs;
    config_file >> num_of_runs >> config.num_threads >> config.total_cores >> config.num_users >> config.timeout >> config.max_time >> config.request_buffer_size >> config.thread_buffer_size >> config.core_buffer_size >> config.core_context_switch_time >> config.core_context_switch_overhead >> think_dist >> service_dist;

    // Initialize distributions based on config
    if (config.think_time_distribution == TimeDistributionType::UNIFORM) {
        double min_think_time, max_think_time;
        config_file >> min_think_time >> max_think_time;
        think_time_dist = new UniformDistribution(min_think_time, max_think_time);
    } else if (config.think_time_distribution == TimeDistributionType::EXPONENTIAL) {
        double lambda;
        config_file >> lambda;
        think_time_dist = new ExponentialDistribution(lambda);
    } else {
        double constant_time;
        config_file >> constant_time;
        think_time_dist = new ConstDistribution(constant_time);
    }

    if (config.service_time_distribution == TimeDistributionType::UNIFORM) {
        double min_service_time, max_service_time;
        config_file >> min_service_time >> max_service_time;
        service_time_dist = new UniformDistribution(min_service_time, max_service_time);
    } else if (config.service_time_distribution == TimeDistributionType::EXPONENTIAL) {
        double lambda;
        config_file >> lambda;
        service_time_dist = new ExponentialDistribution(lambda);
    } else {
        double constant_time;
        config_file >> constant_time;
        service_time_dist = new ConstDistribution(constant_time);
    }
}

void Sim::run() {
    // Schedule initial arrival events for all users
    double current_time = 0;
    for (int i = 0; i < num_users; i++) {
        Request* new_request = new Request(i + 1, current_time, service_time_dist->sample());
        Event* arrival_event = new Event(current_time, EventType::ARRIVAL, new_request);
        event_queue.push(arrival_event);
    }

    std::ofstream output_file("event_log.txt");

    output_file << "------------------------" << std::endl;

    while (!event_queue.empty() && event_queue.top()->timestamp < max_time) {
        output_file << "Current Time: " << event_queue.top()->timestamp << std::endl;
        Event* current_event = event_queue.top();
        event_queue.pop();
        // Print the current event details to the output file
        output_file << "Event Type: " << event_type_to_string(current_event->type) << std::endl;
        // Print request id, thread id, core id if they exist
        if (current_event->request) {
            output_file << "Request ID: " << current_event->request->id << std::endl;
        }
        if (current_event->thread) {
            output_file << "Thread ID: " << current_event->thread->id << std::endl;
        }
        if (current_event->core) {
            output_file << "Core ID: " << current_event->core->id << std::endl;
        }

        output_file << "------------------------" << std::endl;
        // Process the current event based on its type
        switch (current_event->type) {
            case EventType::ARRIVAL: {
                // Assign request to an idle thread if available, otherwise it waits in the receiver's request queue
                if (receiver.thread_pool.has_idle_thread()) {
                    Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                    idle_thread->current_request = current_event->request;
                    // Schedule Thread Arrive event for this thread
                    Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, current_event->request, idle_thread);
                    event_queue.push(thread_arrival_event);
                } else {
                    // If request buffer is full, the request is dropped (not added to the queue)
                    if ((int)receiver.request_queue.size() < receiver.receiver_buffer_size) {
                        receiver.request_queue.push(current_event->request);
                    } else {
                        // Request is dropped, we can log this if needed
                    }
                }
                // Create a time out event for this request
                Event* timeout_event = new Event(current_event->timestamp + timeout, EventType::TIMEOUT, current_event->request);
                event_queue.push(timeout_event);
                all_requests.push_back(current_event->request);
                break;
            }
            case EventType::TIMEOUT: {
                // If the request not yet departed, mark it as timed out and free the thread if it was assigned to one
                if (current_event->request->departure_time < 0) { // Request has not departed yet
                    current_event->request->timed_out = true;
                }
                break;
            }
            // In Thread Arrival event
            // If there is a free core, and the core has no threads yet, the thread starts processing immediately (schedule a thread process event that decides if it will context switch or depart)
            // If there is a free core, but the core already has threads in its buffer, or is busy processing another thread, the thread goes into the core's thread buffer
            // If there is no free core, the thread goes into the worker's thread queue if there is space, otherwise it is dropped (not added to the queue)
            case EventType::THREAD_ARRIVAL: {
                if (worker.has_free_core()) {
                    Core* assigned_core = worker.find_free_core();
                    if (!assigned_core->busy && assigned_core->thread_buffer.empty()) {
                        // Schedule a thread process event for this thread to start processing
                        Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, assigned_core);
                        event_queue.push(thread_process_event);
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
            // If the service time remaining for the request is less than the core context switch time, the thread will finish processing and schedule a departure event
            // If the service time remaining for the request is greater than or equal to the core context switch time, the thread will be preempted after the context switch time and put back into the core's thread buffer, and a context switch event will be scheduled
            case EventType::THREAD_PROCESS: {
                double service_time_remaining = current_event->request->remaining_service_time;
                if (service_time_remaining <= current_event->core->core_context_switch_time) {
                    // Schedule departure event
                    Event* departure_event = new Event(current_event->timestamp + service_time_remaining, EventType::DEPARTURE, current_event->request, current_event->thread, current_event->core);
                    event_queue.push(departure_event);
                    current_event->core->busy = true;
                    worker.busy_cores++;
                    current_event->request->remaining_service_time = 0;
                } else {
                    // Schedule context switch event
                    Event* context_switch_event = new Event(current_event->timestamp + current_event->core->core_context_switch_time, EventType::CONTEXT_SWITCH, current_event->request, current_event->thread, current_event->core);
                    event_queue.push(context_switch_event);
                    current_event->core->busy = true;
                    worker.busy_cores++;
                    current_event->request->remaining_service_time -= current_event->core->core_context_switch_time;
                }
                break;
            }
            // If there are threads waiting in the core's thread buffer, we pop the next thread and schedule a thread process event for it after core context switch overhead time (since the core will be busy with context switching for that duration and cannot start processing the next thread until then)
            // If there are no threads waiting in the core's thread buffer, we schedule a thread process event for the current thread instantly
            case EventType::CONTEXT_SWITCH: {
                if (!current_event->core->thread_buffer.empty()) {
                    Thread* next_thread = current_event->core->thread_buffer.front();
                    current_event->core->thread_buffer.pop();
                    Event* thread_process_event = new Event(current_event->timestamp + current_event->core->core_context_switch_overhead, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core);
                    event_queue.push(thread_process_event);
                    current_event->core->thread_buffer.push(current_event->thread); // Put the current thread back into the core's thread buffer
                } else {
                    // No waiting threads in the core's buffer, schedule a thread process event for the current thread to continue processing immediately
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread, current_event->core);
                    event_queue.push(thread_process_event);
                }
                break;                
            }
            // Before everything, assign the departure time for the request as the current event timestamp, since the request is departing at this time
            // If there is a thread waiting in the core's thread buffer, we schedule a thread process event for that thread to start processing immediately
            // If there are no threads waiting in the core's thread buffer, we mark the core as not busy and decrease the count of busy cores in the worker
            // After that, if there are threads waiting in the worker's thread queue, we pop the next thread and assign it to the core that just got free
            // Note that we don't schedule any event for the thread that we pop from the worker's thread queue, since that thread is already in the core's thread buffer and will be scheduled to process when it reaches the front of the buffer
            // If there is a request waiting in the receiver's request queue, we assign it to the thread that just got free and schedule a thread arrival event for that thread to enter the core buffer
            // Make a arrival event for the same user after think time
            case EventType::DEPARTURE: {
                current_event->request->departure_time = current_event->timestamp;
                current_event->core->busy = false;
                worker.busy_cores--;
                current_event->thread->current_request = nullptr; // Free the thread
                if (!current_event->core->thread_buffer.empty()) {
                    Thread* next_thread = current_event->core->thread_buffer.front();
                    current_event->core->thread_buffer.pop();
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core);
                    event_queue.push(thread_process_event);
                    worker.busy_cores++;
                    current_event->core->busy = true;
                }
                if (!worker.thread_queue.empty()) {
                    Thread* next_thread = worker.thread_queue.front();
                    worker.thread_queue.pop();
                    current_event->core->thread_buffer.push(next_thread);
                    if (!current_event->core->busy) {
                        Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, next_thread->current_request, next_thread, current_event->core);
                        event_queue.push(thread_process_event);
                        worker.busy_cores++;
                        current_event->core->busy = true;
                    }
                } 
                if (!receiver.request_queue.empty()) {
                    Request* next_request = receiver.request_queue.front();
                    receiver.request_queue.pop();
                    Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                    idle_thread->current_request = next_request;
                    Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, next_request, idle_thread);
                    event_queue.push(thread_arrival_event);
                }
                // Schedule next arrival event for the same user after think time
                double think_time = think_time_dist->sample();
                Request* new_request = new Request(current_event->request->user_id, current_event->timestamp + think_time, service_time_dist->sample());
                Event* arrival_event = new Event(current_event->timestamp + think_time, EventType::ARRIVAL, new_request);
                event_queue.push(arrival_event);
                break;
            }
        }
        
        delete current_event; // Clean up the processed event
    }

    // Clean up dynamically allocated events in the event queue
    while (!event_queue.empty()) {
        delete event_queue.top();
        event_queue.pop();
    }

    // Clean up the cores and threads in the worker and receiver
    for (Core* core : worker.cores) {
        while (!core->thread_buffer.empty()) {
            Thread* thread = core->thread_buffer.front();
            core->thread_buffer.pop();
            thread->current_request = nullptr; // Free the thread
        }
    }

    receiver.request_queue = std::queue<Request*>();
    worker.thread_queue = std::queue<Thread*>();

    // Free all threads
    for (Thread* thread : receiver.thread_pool.threads) {
        thread->current_request = nullptr;
    }

    // Number of busy cores at the end of the simulation should be 0
    worker.busy_cores = 0;
    Request::id_counter = 0;
    Thread::id_counter = 0;

    output_file.close();
}

void Sim::print_config() {
    std::cout << "======================" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Number of Threads: " << receiver.thread_pool.threads.size() << std::endl;
    std::cout << "Total Cores: " << worker.total_cores << std::endl;
    std::cout << "Request Buffer Size: " << receiver.receiver_buffer_size << std::endl;
    std::cout << "Thread Buffer Size: " << worker.thread_buffer_size << std::endl;
    std::cout << "Core Buffer Size: " << worker.cores[0]->thread_buffer_size << std::endl;
    std::cout << "Core Context Switch Time: " << worker.core_context_switch_time << std::endl;
    std::cout << "Core Context Switch Overhead: " << worker.core_context_switch_overhead << std::endl;
    std::cout << "Number of Users: " << num_users << std::endl;
    std::cout << "Timeout: " << timeout << std::endl;
    std::cout << "Max Simulation Time: " << max_time << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Think Time Distribution: " << std::endl;
    if (dynamic_cast<UniformDistribution*>(think_time_dist)) {
        std::cout << "Uniform" << std::endl;
        std::cout << "Min Think Time: " << dynamic_cast<UniformDistribution*>(think_time_dist)->a << std::endl;
        std::cout << "Max Think Time: " << dynamic_cast<UniformDistribution*>(think_time_dist)->b << std::endl;
    } else if (dynamic_cast<ExponentialDistribution*>(think_time_dist)) {
        std::cout << "Exponential" << std::endl;
        std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(think_time_dist)->mean << std::endl;
    } else if (dynamic_cast<ConstDistribution*>(think_time_dist)) {
        std::cout << "Deterministic" << std::endl;
        std::cout << "Think Time: " << dynamic_cast<ConstDistribution*>(think_time_dist)->value << std::endl;
    } else {
        std::cout << "Unknown" << std::endl;
    }
    std::cout << "======================" << std::endl;
    std::cout << "Service Time Distribution: " << std::endl;
    if (dynamic_cast<UniformDistribution*>(service_time_dist)) {
        std::cout << "Uniform" << std::endl;
        std::cout << "Min Service Time: " << dynamic_cast<UniformDistribution*>(service_time_dist)->a << std::endl;
        std::cout << "Max Service Time: " << dynamic_cast<UniformDistribution*>(service_time_dist)->b << std::endl;
    } else if (dynamic_cast<ExponentialDistribution*>(service_time_dist)) {
        std::cout << "Exponential" << std::endl;
        std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(service_time_dist)->mean << std::endl;
    } else if (dynamic_cast<ConstDistribution*>(service_time_dist)) {
        std::cout << "Deterministic" << std::endl;
        std::cout << "Service Time: " << dynamic_cast<ConstDistribution*>(service_time_dist)->value << std::endl;
    } else {
        std::cout << "Unknown" << std::endl;
    }
    std::cout << "======================" << std::endl;
    std::cout << std::endl;
}

std::tuple<double, double, double, double, double, double> Sim::print_stats() {
    // Calculate and print statistics such as average response time, throughput, etc.
    double total_response_time = 0;
    int bad_completed_requests = 0;
    int good_completed_requests = 0;
    int dropped_requests = 0;
    double total_cpu_time = 0;
    for (Request* request : all_requests) {
        if (request->departure_time >= 0) { // Only consider completed requests
            total_cpu_time += request->service_time;
            total_response_time += (request->departure_time - request->arrival_time);
            if (!request->timed_out) {
                good_completed_requests++;
            } else {
                bad_completed_requests++;
            }
        }
        else {
            dropped_requests++;
        }
    }
    double average_response_time = (good_completed_requests + bad_completed_requests) > 0 ? total_response_time / (good_completed_requests + bad_completed_requests) : 0;
    // std::cout << "======================" << std::endl;
    // std::cout << "Simulation Statistics:" << std::endl;
    // std::cout << "======================" << std::endl;
    // std::cout << "Average Response Time: " << average_response_time << " sec" << std::endl;
    // std::cout << "-----------------------" << std::endl;
    // std::cout << "Goodput: " << good_completed_requests / max_time << " req/sec" << std::endl;
    // std::cout << "Badput: " << bad_completed_requests / max_time << " req/sec" << std::endl;
    // std::cout << "Throughput: " << (good_completed_requests + bad_completed_requests) / max_time << " req/sec" << std::endl;
    // std::cout << "-----------------------" << std::endl;
    // std::cout << "Average CPU Utilization: " << total_cpu_time / max_time << "%" << std::endl;
    // std::cout << "-----------------------" << std::endl;
    // std::cout << "Request Drop Rate: " << dropped_requests / max_time << " req/sec" << std::endl;
    // std::cout << "======================" << std::endl;

    // Clean up all requests
    for (Request* request : all_requests) {
        delete request;
    }

    all_requests.clear();

    return {average_response_time, good_completed_requests / max_time, bad_completed_requests / max_time, (good_completed_requests + bad_completed_requests) / max_time, total_cpu_time / max_time, dropped_requests / max_time};
}

Sim::~Sim() {
    delete think_time_dist;
    delete service_time_dist;
}