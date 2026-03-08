#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>

class Request{
    inline static int id_counter = 0;
public:
    int id;
    double arrival_time;
    double service_time;
    double departure_time;
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
public:
    std::vector<Thread*> threads;
    ThreadPool(int num_threads);
    ~ThreadPool();
    Thread* find_idle_thread();
    bool has_idle_thread();
};

class Receiver{
public:
    std::queue<Request*> request_queue;
    ThreadPool thread_pool;
    int receiver_buffer_size;
    Receiver(int num_threads, int receiver_buffer_size);
    ~Receiver();
};

class Worker{
public:
    int total_cores;
    int thread_buffer_size;
    int busy_cores;
    std::queue<Thread*> thread_queue;
    Worker(int total_cores, int thread_buffer_size);
    ~Worker();
};

class Distribution{
public:
    virtual double sample() = 0; // Pure virtual function to sample from the distribution
    virtual ~Distribution() = default; // Virtual destructor for proper cleanup of derived classes
};

class ExponentialDistribution : public Distribution{
public:
    double mean;
    ExponentialDistribution(double mean);
    ~ExponentialDistribution();
    double sample() override;
};

class UniformDistribution : public Distribution{
public:
    double a, b;
    UniformDistribution(double a, double b);
    ~UniformDistribution();
    double sample() override;
};

class ConstDistribution : public Distribution{
public:
    double value;
    ConstDistribution(double value);
    ~ConstDistribution();
    double sample() override;
};

#endif