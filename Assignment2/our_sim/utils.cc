#include "commons.hh"

Request::Request(double arrival_time, double service_time) : arrival_time(arrival_time), service_time(service_time) {
    id = id_counter++;
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

Receiver::Receiver(int num_threads) : thread_pool(num_threads) {}

Worker::Worker(int total_cores) : total_cores(total_cores), busy_cores(0) {};
