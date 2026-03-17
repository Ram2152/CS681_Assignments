#include "common.hh"

// Calculate confidence intervals for response time data
std::tuple<double, double> calculate_confidence_interval(const std::vector<double>& data, double confidence_level) {
    int n = data.size();
    if (n == 0) {
        return std::make_tuple(0, 0); // No data, return (0, 0) as confidence interval
    }
    
    // Calculate mean
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / n;

    // Calculate standard deviation
    double variance = std::accumulate(data.begin(), data.end(), 0.0, [mean](double acc, double x) {
        return acc + (x - mean) * (x - mean);
    }) / n;
    double stddev = std::sqrt(variance);

    // Z-score for the given confidence level (e.g., 1.96 for 95% confidence)
    double z_score;
    if (confidence_level == 0.95) {
        z_score = 1.96;
    } else if (confidence_level == 0.90) {
        z_score = 1.645;
    } else if (confidence_level == 0.99) {
        z_score = 2.576;
    } else {
        std::cerr << "Unsupported confidence level! Using 95% confidence by default." << std::endl;
        z_score = 1.96;
    }

    // Calculate margin of error
    double margin_of_error = z_score * (stddev / std::sqrt(n));

    // Calculate confidence interval
    double lower_bound = mean - margin_of_error;
    double upper_bound = mean + margin_of_error;

    return std::make_tuple(lower_bound, upper_bound);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    
    std::ifstream infile(config_file);
    if (!infile.good()) {
        std::cerr << "Error: Config file not found!" << std::endl;
        return 1;
    }

    int num_of_runs;
    infile >> num_of_runs;
    infile.close();

    NetworkSim sim(config_file);
    // sim.print_config();

    // Vectors to store stats for all runs and for all user counts

    std::vector<std::vector<double>> avg_response_times(40, std::vector<double>()); // 40 user counts from 5 to 200 with step of 5
    std::vector<std::vector<double>> good_throughputs(40, std::vector<double>());
    std::vector<std::vector<double>> bad_throughputs(40, std::vector<double>());
    std::vector<std::vector<double>> total_throughputs(40, std::vector<double>());
    std::vector<std::vector<double>> drop_rates(40, std::vector<double>());
    std::vector<std::vector<std::vector<std::tuple<int, int, double>>>> all_cpu_times(40, std::vector<std::vector<std::tuple<int, int, double>>>());

    // In a run, iterate from 5 users to 200 users, and for each user count, call sim.run() and sim.print_stats() to get the stats and write it to a csv file. The csv file should have the following columns: user_count, avg_response_time, good_throughput, bad_throughput, total_throughput, drop_percentage, server_0_utilization, server_1_utilization, server_2_utilization, server_3_utilization. The server_x_utilization columns should contain the overall utilization of each server node (total busy time of all cores divided by (number of cores * max_time)). The csv file should be named "simulation_results.csv". After all runs are done, calculate the confidence intervals for the average response time at 90%, 95%, and 99% confidence levels and print them to the console.

    std::ofstream csv_file("simulation_results.csv");
    csv_file << "user_count,avg_response_time,good_throughput,bad_throughput,total_throughput,drop_percentage";
    for (int server_id = 0; server_id < 4; server_id++) {
        csv_file << ",server_" << server_id << "_utilization";
    }
    csv_file << std::endl;

    for (int user_count = 5; user_count <= 200; user_count += 5) {
        sim.set_num_users(0, user_count);
        for (int run = 0; run < num_of_runs; run++) {
            std::cout << "Run " << run + 1 << " User Count: " << user_count << std::endl;
    
            sim.run();
            auto stats = sim.print_stats();
            avg_response_times[(user_count - 5) / 5].push_back(std::get<0>(stats));
            good_throughputs[(user_count - 5) / 5].push_back(std::get<1>(stats));
            bad_throughputs[(user_count - 5) / 5].push_back(std::get<2>(stats));
            total_throughputs[(user_count - 5) / 5].push_back(std::get<3>(stats));
            drop_rates[(user_count - 5) / 5].push_back(std::get<4>(stats));
            all_cpu_times[(user_count - 5) / 5].push_back(std::get<5>(stats));
        }

        double average_resp_time = 0;
        double average_good_throughput = 0;
        double average_bad_throughput = 0;
        double average_total_throughput = 0;
        double average_drop_percentage = 0;
        std::vector<std::tuple<int, double>> average_server_utilizations; // server_id -> average utilization percentage
    
        int index = (user_count - 5) / 5;
        average_resp_time = std::accumulate(avg_response_times[index].begin(), avg_response_times[index].end(), 0.0) / avg_response_times[index].size();
        average_good_throughput = std::accumulate(good_throughputs[index].begin(), good_throughputs[index].end(), 0.0) / good_throughputs[index].size();
        average_bad_throughput = std::accumulate(bad_throughputs[index].begin(), bad_throughputs[index].end(), 0.0) / bad_throughputs[index].size();
        average_total_throughput = std::accumulate(total_throughputs[index].begin(), total_throughputs[index].end(), 0.0) / total_throughputs[index].size();
        average_drop_percentage = std::accumulate(drop_rates[index].begin(), drop_rates[index].end(), 0.0) / drop_rates[index].size();
        
        // Calculate average server utilization across all runs
        std::map<int, std::vector<double>> server_utilizations; // server_id -> vector of utilizations across runs
        for (const auto& run_cpu_times : all_cpu_times[index]) {
            for (const auto& entry : run_cpu_times) {
                int server_id = std::get<0>(entry);
                int core_id = std::get<1>(entry);
                double busy_time = std::get<2>(entry);
                double utilization = (busy_time / sim.max_time) * 100;
                server_utilizations[server_id].push_back(utilization);
            }
        }
        for (const auto& server_entry : server_utilizations) {
            int server_id = server_entry.first;
            const auto& utilizations = server_entry.second;
            double average_utilization = std::accumulate(utilizations.begin(), utilizations.end(), 0.0) / utilizations.size();
            average_server_utilizations.push_back({server_id, average_utilization});
        }
        // Write to CSV
        csv_file << user_count << "," << average_resp_time << "," << average_good_throughput << "," << average_bad_throughput << "," << average_total_throughput << "," << average_drop_percentage;
        for (int server_id = 0; server_id < 4; server_id++) {
            auto it = std::find_if(average_server_utilizations.begin(), average_server_utilizations.end(), [server_id](const std::tuple<int, double>& entry) {
                return std::get<0>(entry) == server_id;
            });
            if (it != average_server_utilizations.end()) {
                csv_file << "," << std::get<1>(*it);
            } else {
                csv_file << ",0"; // If no data for this server, write 0
            }
        }
        csv_file << std::endl;
        auto [ci_90_lower, ci_90_upper] = calculate_confidence_interval(avg_response_times[index], 0.90);
        auto [ci_95_lower, ci_95_upper] = calculate_confidence_interval(avg_response_times[index], 0.95);
        auto [ci_99_lower, ci_99_upper] = calculate_confidence_interval(avg_response_times[index], 0.99);
    }
    
    // for (int run = 0; run < num_of_runs; run++) {
    //     std::cout << "======================" << std::endl;
    //     std::cout << "Run " << run + 1 << " of " << num_of_runs << " Summary:" << std::endl;
    //     std::cout << "Average Response Time: " << avg_response_times[run] << " seconds" << std::endl;
    //     std::cout << "Good Throughput: " << good_throughputs[run] << " req/sec" << std::endl;
    //     std::cout << "Bad Throughput: " << bad_throughputs[run] << " req/sec" << std::endl;
    //     std::cout << "Total Throughput: " << total_throughputs[run] << " req/sec" << std::endl;
    //     std::cout << "Request Drop Percentage: " << drop_rates[run] * 100 << " %" << std::endl;
    //     // For each core, divide the total busy time by max_time to get the CPU utilization percentage
    //     // For each server node, print the CPU utilization of each core
    //     // For each server node, print the CPU utilization of the server node as a whole (total busy time of all cores divided by (number of cores * max_time))
    //     // Iterate through all servernodes, and for each core, print the total busy time and utilization percentage
    //     std::cout << "CPU Utilization:" << std::endl;
    //     std::vector<std::tuple<int, int, double>> cpu_times = all_cpu_times[run];
    //     std::map<int, std::vector<std::tuple<int, double>>> server_core_times; // server_id -> vector of (core_id, busy_time)
    //     for (const auto& entry : cpu_times) {
    //         int server_id = std::get<0>(entry);
    //         int core_id = std::get<1>(entry);
    //         double busy_time = std::get<2>(entry);
    //         server_core_times[server_id].push_back({core_id, busy_time});
    //     }
    //     for (const auto& server_entry : server_core_times) {
    //         int server_id = server_entry.first;
    //         const auto& core_times = server_entry.second;
    //         double total_busy_time = 0;
    //         std::cout << "  Server Node " << server_id << ":" << std::endl;
    //         for (const auto& core_entry : core_times) {
    //             int core_id = std::get<0>(core_entry);
    //             double busy_time = std::get<1>(core_entry);
    //             total_busy_time += busy_time;
    //             double utilization = (busy_time / sim.max_time) * 100;
    //             std::cout << "    Core " << core_id << ": Busy Time = " << busy_time << " seconds, Utilization = " << utilization << "%" << std::endl;
    //         }
    //         double server_utilization = (total_busy_time / (core_times.size() * sim.max_time)) * 100;
    //         std::cout << "  Server Node " << server_id << " Overall Utilization: " << server_utilization << "%" << std::endl;   
    //     }
    
    //     std::cout << "======================" << std::endl;
    //     std::cout << std::endl;
    // }
    
    // For every user count, go through all runs and calculate the confidence intervals for the average response time at 90%, 95%, and 99% confidence levels, and store it
    // Also calculate the server utilization across all runs and take the average of it, and store it as well

    // for (int user_count = 5; user_count <= 200; user_count += 5) {
    //     int index = (user_count - 5) / 5;
    //     average_resp_time = std::accumulate(avg_response_times[index].begin(), avg_response_times[index].end(), 0.0) / avg_response_times[index].size();
    //     average_good_throughput = std::accumulate(good_throughputs[index].begin(), good_throughputs[index].end(), 0.0) / good_throughputs[index].size();
    //     average_bad_throughput = std::accumulate(bad_throughputs[index].begin(), bad_throughputs[index].end(), 0.0) / bad_throughputs[index].size();
    //     average_total_throughput = std::accumulate(total_throughputs[index].begin(), total_throughputs[index].end(), 0.0) / total_throughputs[index].size();
    //     average_drop_percentage = std::accumulate(drop_rates[index].begin(), drop_rates[index].end(), 0.0) / drop_rates[index].size();
        
    //     // Calculate average server utilization across all runs
    //     std::map<int, std::vector<double>> server_utilizations; // server_id -> vector of utilizations across runs
    //     for (const auto& run_cpu_times : all_cpu_times[index]) {
    //         for (const auto& entry : run_cpu_times) {
    //             int server_id = std::get<0>(entry);
    //             int core_id = std::get<1>(entry);
    //             double busy_time = std::get<2>(entry);
    //             double utilization = (busy_time / sim.max_time) * 100;
    //             server_utilizations[server_id].push_back(utilization);
    //         }
    //     }
    //     for (const auto& server_entry : server_utilizations) {
    //         int server_id = server_entry.first;
    //         const auto& utilizations = server_entry.second;
    //         double average_utilization = std::accumulate(utilizations.begin(), utilizations.end(), 0.0) / utilizations.size();
    //         average_server_utilizations.push_back({server_id, average_utilization});
    //     }
    //     // Write to CSV
    //     csv_file << user_count << "," << average_resp_time << "," << average_good_throughput << "," << average_bad_throughput << "," << average_total_throughput << "," << average_drop_percentage;
    //     for (int server_id = 0; server_id < 4; server_id++) {
    //         auto it = std::find_if(average_server_utilizations.begin(), average_server_utilizations.end(), [server_id](const std::tuple<int, double>& entry) {
    //             return std::get<0>(entry) == server_id;
    //         });
    //         if (it != average_server_utilizations.end()) {
    //             csv_file << "," << std::get<1>(*it);
    //         } else {
    //             csv_file << ",0"; // If no data for this server, write 0
    //         }
    //     }
    //     csv_file << std::endl;
    //     auto [ci_90_lower, ci_90_upper] = calculate_confidence_interval(avg_response_times[index], 0.90);
    //     auto [ci_95_lower, ci_95_upper] = calculate_confidence_interval(avg_response_times[index], 0.95);
    //     auto [ci_99_lower, ci_99_upper] = calculate_confidence_interval(avg_response_times[index], 0.99);
    // }
    
    
    return 0;
}