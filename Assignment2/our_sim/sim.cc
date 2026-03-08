#include "common.hh"

Sim::Sim(Config config) : receiver(config.num_threads, config.request_buffer_size), worker(config.total_cores, config.thread_buffer_size) {
    std::ifstream config_file(config.input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }

    std::string arrival_dist, service_dist;
    config_file >> config.num_threads >> config.total_cores >> config.request_buffer_size >> config.thread_buffer_size >> arrival_dist >> service_dist;

    // Initialize distributions based on config
    if (config.arrival_time_distribution == TimeDistributionType::UNIFORM) {
        float min_arrival_time, max_arrival_time;
        config_file >> min_arrival_time >> max_arrival_time;
        inter_arrival_time_dist = new UniformDistribution(min_arrival_time, max_arrival_time);
    } else if (config.arrival_time_distribution == TimeDistributionType::EXPONENTIAL) {
        float lambda;
        config_file >> lambda;
        inter_arrival_time_dist = new ExponentialDistribution(lambda);
    } else {
        float constant_time;
        config_file >> constant_time;
        inter_arrival_time_dist = new ConstDistribution(constant_time);
    }

    if (config.service_time_distribution == TimeDistributionType::UNIFORM) {
        float min_service_time, max_service_time;
        config_file >> min_service_time >> max_service_time;
        service_time_dist = new UniformDistribution(min_service_time, max_service_time);
    } else if (config.service_time_distribution == TimeDistributionType::EXPONENTIAL) {
        float lambda;
        config_file >> lambda;
        service_time_dist = new ExponentialDistribution(lambda);
    } else {
        float constant_time;
        config_file >> constant_time;
        service_time_dist = new ConstDistribution(constant_time);
    }

    // Schedule the first five arrival events to kickstart the simulation
    for (int i = 0; i < 5; i++) {
        float arrival_time_difference = inter_arrival_time_dist->sample();
        float service_time = service_time_dist->sample();
        Request* first_request = new Request(last_arrival_time + arrival_time_difference, service_time);
        Event* first_event = new Event(last_arrival_time + arrival_time_difference, EventType::ARRIVAL, first_request);
        event_queue.push(first_event);
        last_arrival_time += arrival_time_difference;
    }
}

void Sim::run() {
    while (!event_queue.empty()) {
        Event* current_event = event_queue.top();
        event_queue.pop();
        
        // Process the current event based on its type
        switch (current_event->type) {
            case EventType::ARRIVAL: {
                // Handle arrival event
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
                all_requests.push_back(current_event->request);
                break;
            }
            case EventType::TIMEOUT: {
                // Handle timeout event
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
                float departure_time = current_event->timestamp + current_event->thread->current_request->service_time;
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
    std::cout << "Configuration:" << std::endl;
    std::cout << "Number of Threads: " << receiver.thread_pool.threads.size() << std::endl;
    std::cout << "Total Cores: " << worker.total_cores << std::endl;
    std::cout << "Request Buffer Size: " << receiver.receiver_buffer_size << std::endl;
    std::cout << "Thread Buffer Size: " << worker.thread_buffer_size << std::endl;
    std::cout << "Inter Arrival Time Distribution: ";
    if (dynamic_cast<UniformDistribution*>(inter_arrival_time_dist)) {
        std::cout << "Uniform" << std::endl;
        std::cout << "Min Arrival Time: " << dynamic_cast<UniformDistribution*>(inter_arrival_time_dist)->a << std::endl;
        std::cout << "Max Arrival Time: " << dynamic_cast<UniformDistribution*>(inter_arrival_time_dist)->b << std::endl;
    } else if (dynamic_cast<ExponentialDistribution*>(inter_arrival_time_dist)) {
        std::cout << "Exponential" << std::endl;
        std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(inter_arrival_time_dist)->mean << std::endl;
    } else if (dynamic_cast<ConstDistribution*>(inter_arrival_time_dist)) {
        std::cout << "Arrival Time: " << dynamic_cast<ConstDistribution*>(inter_arrival_time_dist)->value << std::endl;
        std::cout << "Deterministic" << std::endl;
    } else {
        std::cout << "Unknown" << std::endl;
    }
    std::cout << "Service Time Distribution: ";
    if (dynamic_cast<UniformDistribution*>(service_time_dist)) {
        std::cout << "Min Service Time: " << dynamic_cast<UniformDistribution*>(service_time_dist)->a << std::endl;
        std::cout << "Max Service Time: " << dynamic_cast<UniformDistribution*>(service_time_dist)->b << std::endl;
        std::cout << "Uniform" << std::endl;
    } else if (dynamic_cast<ExponentialDistribution*>(service_time_dist)) {
        std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(service_time_dist)->mean << std::endl;
        std::cout << "Exponential" << std::endl;
    } else if (dynamic_cast<ConstDistribution*>(service_time_dist)) {
        std::cout << "Service Time: " << dynamic_cast<ConstDistribution*>(service_time_dist)->value << std::endl;
        std::cout << "Deterministic" << std::endl;
    } else {
        std::cout << "Unknown" << std::endl;
    }
}

void Sim::print_stats() {
    // Calculate and print statistics such as average response time, throughput, etc.
    double total_response_time = 0;
    int completed_requests = 0;
    for (Request* request : all_requests) {
        if (request->departure_time >= 0) { // Only consider completed requests
            total_response_time += (request->departure_time - request->arrival_time);
            completed_requests++;
        }
    }
    double average_response_time = completed_requests > 0 ? total_response_time / completed_requests : 0;
    std::cout << "Average Response Time: " << average_response_time << std::endl;
    std::cout << "Throughput: " << completed_requests << " requests processed." << std::endl;
}

Sim::~Sim() {
    // Clean up dynamically allocated events in the event queue
    while (!event_queue.empty()) {
        delete event_queue.top();
        event_queue.pop();
    }
    delete inter_arrival_time_dist;
    delete service_time_dist;

    // Clean up all requests
    for (Request* request : all_requests) {
        delete request;
    }
}