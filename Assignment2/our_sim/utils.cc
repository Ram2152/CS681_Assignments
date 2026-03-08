#include "common.hh"

Request::Request(double arrival_time, double service_time) : arrival_time(arrival_time), service_time(service_time) {
    id = id_counter++;
    departure_time = -1; // Initialize departure time to -1 to indicate it hasn't departed yet
}

Thread::Thread(){
    id = id_counter++;
    current_request = nullptr;
}

bool Thread::idle(){return current_request == nullptr;}

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

Receiver::Receiver(int num_threads, int receiver_buffer_size) : thread_pool(num_threads), receiver_buffer_size(receiver_buffer_size) {}

Worker::Worker(int total_cores, int thread_buffer_size) : total_cores(total_cores), thread_buffer_size(thread_buffer_size), busy_cores(0) {};

ExponentialDistribution::ExponentialDistribution(double mean) : mean(mean) {}
UniformDistribution::UniformDistribution(double a, double b) : a(a), b(b) {}
ConstDistribution::ConstDistribution(double value) : value(value) {}

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