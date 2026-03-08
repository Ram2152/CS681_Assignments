#include "common.hh"

Sim::Sim(Config config) : num_users(config.num_users), timeout(config.timeout), max_time(config.max_time), receiver(config.num_threads, config.request_buffer_size), worker(config.total_cores, config.thread_buffer_size) {
    std::ifstream config_file(config.input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }

    std::string think_dist, service_dist;
    config_file >> config.num_threads >> config.total_cores >> config.num_users >> config.timeout >> config.max_time >> config.request_buffer_size >> config.thread_buffer_size >> think_dist >> service_dist;

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

    // Schedule initial arrival events for all users
    double current_time = 0;
    for (int i = 0; i < num_users; i++) {
        Request* new_request = new Request(i + 1, current_time, service_time_dist->sample());
        Event* arrival_event = new Event(current_time, EventType::ARRIVAL, new_request);
        event_queue.push(arrival_event);
    }
}

void Sim::run() {
    while (!event_queue.empty() && event_queue.top()->timestamp < max_time) {
        Event* current_event = event_queue.top();
        event_queue.pop();

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
            case EventType::THREAD_ARRIVAL: {
                // When a thread arrives, it goes into the worker's thread queue if there are no free cores, otherwise it starts processing immediately
                if (worker.busy_cores < worker.total_cores) {
                    // Make a thread process event immediately for this thread
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, current_event->request, current_event->thread);
                    event_queue.push(thread_process_event);
                } else {
                    // If worker buffer is full, then the thread is dropped (not added to the queue)
                    if ((int)worker.thread_queue.size() < worker.thread_buffer_size) {
                        worker.thread_queue.push(current_event->thread);
                    } else {
                        // Thread is dropped, we can log this if needed (thread is freed but request is not processed, we can consider this as a request drop as well)
                        current_event->thread->current_request = nullptr; // Free the thread
                    }
                }
                break;
            }
            case EventType::THREAD_PROCESS: {
                // Schedule departure event for the request being processed by this thread
                worker.busy_cores++;
                double departure_time = current_event->timestamp + current_event->thread->current_request->service_time;
                Event* departure_event = new Event(departure_time, EventType::DEPARTURE, current_event->thread->current_request, current_event->thread);
                event_queue.push(departure_event);
                break;
            }
            case EventType::DEPARTURE: {
                // Assign departure time to the request
                current_event->request->departure_time = current_event->timestamp;
                // Free the thread
                current_event->thread->current_request = nullptr;
                worker.busy_cores--;

                // Schedule arrival of the next request from the same user after think time
                double next_arrival_time = current_event->timestamp + think_time_dist->sample();
                Request* next_request = new Request(current_event->request->user_id, next_arrival_time, service_time_dist->sample());
                Event* next_arrival_event = new Event(next_arrival_time, EventType::ARRIVAL, next_request);
                event_queue.push(next_arrival_event);
                
                // If there are waiting threads in the worker's thread queue, create a thread process event for the next thread in the queue
                if (!worker.thread_queue.empty()) {
                    Thread* next_thread = worker.thread_queue.front();
                    worker.thread_queue.pop();
                    Event* thread_process_event = new Event(current_event->timestamp, EventType::THREAD_PROCESS, next_thread->current_request, next_thread);
                    event_queue.push(thread_process_event);
                }
                
                // If there are waiting requests in the receiver's request queue, assign the next request to an idle thread and schedule a thread arrival event
                if (!receiver.request_queue.empty()) {
                    Request* next_request = receiver.request_queue.front();
                    receiver.request_queue.pop();
                    // Find an idle thread and assign the request to it
                    Thread* idle_thread = receiver.thread_pool.find_idle_thread();
                    idle_thread->current_request = next_request;
                    Event* thread_arrival_event = new Event(current_event->timestamp, EventType::THREAD_ARRIVAL, next_request, idle_thread);
                    event_queue.push(thread_arrival_event);
                }
                break;
            }
        }
        
        delete current_event; // Clean up the processed event
    }
}

void Sim::print_config() {
    std::cout << "======================" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Number of Threads: " << receiver.thread_pool.threads.size() << std::endl;
    std::cout << "Total Cores: " << worker.total_cores << std::endl;
    std::cout << "Request Buffer Size: " << receiver.receiver_buffer_size << std::endl;
    std::cout << "Thread Buffer Size: " << worker.thread_buffer_size << std::endl;
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

void Sim::print_stats() {
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
    std::cout << "======================" << std::endl;
    std::cout << "Simulation Statistics:" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Average Response Time: " << average_response_time << " sec" << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << "Goodput: " << good_completed_requests / max_time << " req/sec" << std::endl;
    std::cout << "Badput: " << bad_completed_requests / max_time << " req/sec" << std::endl;
    std::cout << "Throughput: " << (good_completed_requests + bad_completed_requests) / max_time << " req/sec" << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << "Average CPU Utilization: " << total_cpu_time / max_time << "%" << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << "Request Drop Rate: " << dropped_requests / max_time << " req/sec" << std::endl;
    std::cout << "======================" << std::endl;
}

Sim::~Sim() {
    // Clean up dynamically allocated events in the event queue
    while (!event_queue.empty()) {
        delete event_queue.top();
        event_queue.pop();
    }
    delete think_time_dist;
    delete service_time_dist;

    // Clean up all requests
    for (Request* request : all_requests) {
        delete request;
    }
}