#ifndef CONFIG_HH
#define CONFIG_HH

enum class TimeDistributionType {
    UNIFORM,
    EXPONENTIAL,
    DETERMINISTIC
};

class Config {
public:
    int num_threads;
    int total_cores;
    TimeDistributionType arrival_time_distribution;
    TimeDistributionType service_time_distribution;
    Config(int num_threads, int total_cores, TimeDistributionType arrival_time_distribution, TimeDistributionType service_time_distribution);
};

#endif