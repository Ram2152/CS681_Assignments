#ifndef CONFIG_HH
#define CONFIG_HH

#include "common.hh"

enum class TimeDistributionType {
    UNIFORM,
    EXPONENTIAL,
    DETERMINISTIC
};

class Config {
public:
    std::string input_file; // To get the configuration parameters from a file instead of command line arguments
    int num_threads;
    int total_cores;
    int request_buffer_size;
    int thread_buffer_size;
    TimeDistributionType arrival_time_distribution;
    TimeDistributionType service_time_distribution;
    Config(std::string input_file);
};

#endif