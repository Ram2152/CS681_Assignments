#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>

class Request{
    inline static int id_counter = 0;
public:
    int id;
    double arrival_time;
    double service_time;
    Request(double arrival_time, double service_time);
    ~Request();
};

class Thread{
    inline static int id_counter = 0;
public:
    int id;
    Request *current_request;
    Thread();
    ~Thread();
    bool idle();
};

class ThreadPool{
    std::vector<Thread*> threads;
public:
    ThreadPool(int num_threads);
    ~ThreadPool();
    Thread* find_idle_thread();
};

class Receiver{
public:
    std::queue<Request*> request_queue;
    ThreadPool thread_pool;
    Receiver(int num_threads);
    ~Receiver();
};

class Worker{
public:
    std::queue<Thread*> thread_queue;
    int total_cores;
    int busy_cores;
    Worker(int total_cores);
    ~Worker();
};

#endif