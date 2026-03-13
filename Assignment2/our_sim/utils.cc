#include "common.hh"

Request::Request(int user_id, double arrival_time, double service_time) : user_id(user_id), arrival_time(arrival_time), service_time(service_time), remaining_service_time(service_time) {
    id = id_counter++;
    timed_out = false;
    is_dropped = false;
    departure_time = -1; // Initialize departure time to -1 to indicate it hasn't departed yet
}

Request::~Request() {}

Thread::Thread(){
    id = id_counter++;
    current_request = nullptr;
}

bool Thread::idle(){return current_request == nullptr;}

Thread::~Thread() {}

ThreadPool::ThreadPool(int num_threads){
    for(int i = 0; i < num_threads; i++){
        threads.emplace_back(new Thread());
    }
}

Thread* ThreadPool::find_idle_thread(){
    for(Thread* thread : threads){
        if(thread->idle()){
            return thread;
        }
    }
    return nullptr;
}

bool ThreadPool::has_idle_thread(){
    for(Thread* thread : threads){
        if(thread->idle()){
            return true;
        }
    }
    return false;
}

ThreadPool::~ThreadPool(){
    for(Thread* thread : threads){
        delete thread;
    }
}

Receiver::Receiver(int num_threads, int receiver_buffer_size) : thread_pool(num_threads), receiver_buffer_size(receiver_buffer_size) {}

Receiver::~Receiver() {}

Worker::Worker(int total_cores, int thread_buffer_size, int core_buffer_size, double core_context_switch_time, double core_context_switch_overhead) : total_cores(total_cores), thread_buffer_size(thread_buffer_size), busy_cores(0), core_context_switch_time(core_context_switch_time), core_context_switch_overhead(core_context_switch_overhead) {
    for(int i = 0; i < total_cores; i++){
        cores.emplace_back(new Core(core_buffer_size, core_context_switch_time, core_context_switch_overhead));
    }
}

Worker::~Worker() {
    while (!thread_queue.empty()) {
        thread_queue.pop();
    }
}

// If some core has free buffer space for threads, we can consider that core as a free core (even if it is currently busy processing a thread, it can still accept more threads in its buffer)

bool Worker::has_free_core() {
    for (Core* core : cores) {
        if (!core->busy || (int)core->thread_buffer.size() < core->thread_buffer_size) {
            return true;
        }
    }
    return false;
}

Core* Worker::find_free_core() {
    Core* idle_core = nullptr;
    int min_buffer_size = std::numeric_limits<int>::max();
    for (Core* core : cores) {
        if (!core->busy) {
            return core; // If we find a completely idle core, return it immediately
        }
        if ((int)core->thread_buffer.size() < min_buffer_size) {
            idle_core = core;
            min_buffer_size = core->thread_buffer.size();
        }
    }
    return idle_core;
}

Core::Core(int thread_buffer_size, double core_context_switch_time, double core_context_switch_overhead) : busy(false), thread_buffer_size(thread_buffer_size), core_context_switch_time(core_context_switch_time), core_context_switch_overhead(core_context_switch_overhead) {
    id = id_counter++;
    total_busy_time = 0;
}

Core::~Core() {
    while (!thread_buffer.empty()) {
        thread_buffer.pop();
    }
}

ExponentialDistribution::ExponentialDistribution(double mean) : mean(mean) {}
UniformDistribution::UniformDistribution(double a, double b) : a(a), b(b) {}
ConstDistribution::ConstDistribution(double value) : value(value) {}
NormalDistribution::NormalDistribution(double mean, double stddev) : mean(mean), stddev(stddev) {}
MultinomialDistribution::MultinomialDistribution(const std::vector<double>& probs)
    : probabilities(probs),
      gen(std::random_device{}()),
      dist(probabilities.begin(), probabilities.end()) {}

ExponentialDistribution::~ExponentialDistribution() {}
UniformDistribution::~UniformDistribution() {}
ConstDistribution::~ConstDistribution() {}
NormalDistribution::~NormalDistribution() {}
MultinomialDistribution::~MultinomialDistribution() {}

double ExponentialDistribution::sample() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> d(1.0 / mean);
    return d(gen);
}

double UniformDistribution::sample() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> d(a, b);
    return d(gen);
}

double ConstDistribution::sample() {
    return value;
}

double NormalDistribution::sample() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(mean, stddev);
    return d(gen);
}

int MultinomialDistribution::sample() {
    return dist(gen);
}