#ifndef UTILS_H
#define UTILS_H

#include "common.hh"

class Request{
public:
    inline static int id_counter = 0;
    int id;
    int user_id;
    bool timed_out;
    bool is_dropped;
    double arrival_time;
    double service_time;
    double remaining_service_time;
    double departure_time;
    Request(int user_id, double arrival_time, double service_time);
    ~Request();
};

class Thread{
public:
    inline static int id_counter = 0;
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

class Core{
public:
    inline static int id_counter = 0;
    int id;
    bool busy;
    int thread_buffer_size;
    double core_context_switch_time;
    double core_context_switch_overhead;
    std::queue<Thread*> thread_buffer;
    Core(int thread_buffer_size, double core_context_switch_time, double core_context_switch_overhead);
    ~Core();
};

class Worker{
public:
    int total_cores;
    int thread_buffer_size;
    int busy_cores;
    double core_context_switch_time;
    double core_context_switch_overhead;
    std::vector<Core*> cores;
    std::queue<Thread*> thread_queue;
    Worker(int total_cores, int thread_buffer_size, int core_buffer_size, double core_context_switch_time, double core_context_switch_overhead);
    bool has_free_core();
    Core* find_free_core();
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

class MultinomialDistribution {
private:
    std::vector<double> probabilities;
    std::mt19937 gen;
    std::discrete_distribution<> dist;

public:
    MultinomialDistribution(const std::vector<double>& probs);
    ~MultinomialDistribution();
    int sample();
};

#endif