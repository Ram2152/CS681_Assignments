#ifndef CONFIG_HH
#define CONFIG_HH

#include "common.hh"

enum class TimeDistributionType {
    UNIFORM,
    EXPONENTIAL,
    DETERMINISTIC
};

// Config file format:
// num_threads total_cores num_users timeout max_time request_buffer_size thread_buffer_size think_dist service_dist [think_dist_params] [service_dist_params]

class Config {
public:
    std::string input_file; // To get the configuration parameters from a file instead of command line arguments
    int num_threads;
    int total_cores;
    int num_users;
    double timeout;
    double max_time;
    int request_buffer_size;
    int thread_buffer_size;
    TimeDistributionType think_time_distribution;
    TimeDistributionType service_time_distribution;
    Config(std::string input_file_name);
};

#endif