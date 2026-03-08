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
    std::queue<Thread*> thread_queue;
    int thread_buffer_size;
    int total_cores;
    int busy_cores;
    Worker(int total_cores, int thread_buffer_size);
    ~Worker();
};

class Distribution{
public:
    virtual double sample();
    Distribution();
    ~Distribution();
};

class ExponentialDistribution : public Distribution{
    double mean;
public:
    ExponentialDistribution(double mean);
    ~ExponentialDistribution();
    double sample() override;
};

class UniformDistribution : public Distribution{
    double a, b;
public:
    UniformDistribution(double a, double b);
    ~UniformDistribution();
    double sample() override;
};

class ConstDistribution : public Distribution{
    double value;
public:
    ConstDistribution(double value);
    ~ConstDistribution();
    double sample() override;
};

#endif