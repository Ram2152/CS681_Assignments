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
    
    simdjson::dom::parser parser;
    simdjson::dom::element config;
    auto error = parser.load(config_file).get(config);
    if (error) {
        std::cerr << "Error parsing config file: " << simdjson::error_message(error) << std::endl;
        exit(1);
    }

    int num_of_runs = config["num_runs"].get_int64();
    std::string_view output_file = config["output_file"].get_string();

    NetworkSim sim(config_file);
    sim.print_config();

    // Vectors to store stats for all runs and for all user counts

    std::vector<std::vector<double>> avg_response_times((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<double>());
    std::vector<std::vector<double>> good_throughputs((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<double>());
    std::vector<std::vector<double>> bad_throughputs((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<double>());
    std::vector<std::vector<double>> total_throughputs((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<double>());
    std::vector<std::vector<double>> drop_rates((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<double>());
    std::vector<std::vector<std::vector<std::tuple<int, int, double>>>> all_cpu_times((sim.max_num_users - sim.min_num_users) / sim.step_size + 1, std::vector<std::vector<std::tuple<int, int, double>>>());

    // In a run, iterate from 5 users to 200 users, and for each user count, call sim.run() and sim.print_stats() to get the stats and write it to a csv file. The csv file should have the following columns: user_count, avg_response_time, good_throughput, bad_throughput, total_throughput, drop_percentage, server_0_utilization, server_1_utilization, server_2_utilization, server_3_utilization. The server_x_utilization columns should contain the overall utilization of each server node (total busy time of all cores divided by (number of cores * max_time)). The csv file should be named "simulation_results.csv". After all runs are done, calculate the confidence intervals for the average response time at 90%, 95%, and 99% confidence levels and print them to the console.

    std::ofstream csv_file(output_file.data());
    csv_file << "user_count,avg_response_time,good_throughput,bad_throughput,total_throughput,drop_percentage,";
    for(int server_id = 0; server_id < (int)sim.server_nodes.size(); server_id++) {
        csv_file << "server_" << server_id << "_utilization,";
    }
    // Add 6 headers for confidence intervals of average response time at 90%, 95%, and 99% confidence levels
    csv_file << "ci_90_lower,ci_90_upper,ci_95_lower,ci_95_upper,ci_99_lower,ci_99_upper";
    csv_file << std::endl;

    for (int user_count = sim.min_num_users; user_count <= sim.max_num_users; user_count += sim.step_size) {
        sim.set_num_users(0, user_count);
        int index = (user_count - sim.min_num_users) / sim.step_size;
        for (int run = 0; run < num_of_runs; run++) {
            std::cout << "Run " << run + 1 << " User Count: " << user_count << std::endl;
    
            sim.run();
            auto stats = sim.print_stats();
            avg_response_times[index].push_back(std::get<0>(stats));
            good_throughputs[index].push_back(std::get<1>(stats));
            bad_throughputs[index].push_back(std::get<2>(stats));
            total_throughputs[index].push_back(std::get<3>(stats));
            drop_rates[index].push_back(std::get<4>(stats));
            all_cpu_times[index].push_back(std::get<5>(stats));
        }

        double average_resp_time = 0;
        double average_good_throughput = 0;
        double average_bad_throughput = 0;
        double average_total_throughput = 0;
        double average_drop_percentage = 0;
        std::vector<double> average_cpu_times(sim.server_nodes.size(), 0);
        std::vector<double> average_server_utilizations(sim.server_nodes.size(), 0);

        average_resp_time = std::accumulate(avg_response_times[index].begin(), avg_response_times[index].end(), 0.0) / avg_response_times[index].size();
        average_good_throughput = std::accumulate(good_throughputs[index].begin(), good_throughputs[index].end(), 0.0) / good_throughputs[index].size();
        average_bad_throughput = std::accumulate(bad_throughputs[index].begin(), bad_throughputs[index].end(), 0.0) / bad_throughputs[index].size();
        average_total_throughput = std::accumulate(total_throughputs[index].begin(), total_throughputs[index].end(), 0.0) / total_throughputs[index].size();
        average_drop_percentage = std::accumulate(drop_rates[index].begin(), drop_rates[index].end(), 0.0) / drop_rates[index].size();
        for(int server_id = 0; server_id < (int)sim.server_nodes.size(); server_id++) {
            double average_busy_time = 0;
            for (const auto& run_cpu_times : all_cpu_times[index]) {
                for (const auto& entry : run_cpu_times) {
                    int s_id = std::get<0>(entry);
                    double busy_time = std::get<2>(entry);
                    // std::cerr << "Server " << s_id << " Core " << core_id << " Busy Time in this run: " << busy_time << " seconds" << std::endl;
                    if (s_id == server_id + (int)sim.client_nodes.size()) {
                        average_busy_time += busy_time;
                    }
                }
            }
            // std::cout << "Average Busy Time for Server " << server_id << ": " << average_busy_time << " seconds" << std::endl;
            average_busy_time /= all_cpu_times[index].size(); // Average busy time across all runs for this server
            // std::cerr << "Average Busy Time for Server " << server_id << ": " << average_busy_time << " seconds" << std::endl;
            average_cpu_times[server_id] = average_busy_time;
            average_server_utilizations[server_id] = (average_busy_time / (sim.server_nodes[server_id]->worker.cores.size() * sim.max_time)) * 100;
        }

        // Write to CSV
        csv_file << user_count << "," << average_resp_time << "," << average_good_throughput << "," << average_bad_throughput << "," << average_total_throughput << "," << average_drop_percentage;
        for(int server_id = 0; server_id < (int)sim.server_nodes.size(); server_id++) {
            csv_file << "," << average_server_utilizations[server_id];
        }
        auto [ci_90_lower, ci_90_upper] = calculate_confidence_interval(avg_response_times[index], 0.90);
        auto [ci_95_lower, ci_95_upper] = calculate_confidence_interval(avg_response_times[index], 0.95);
        auto [ci_99_lower, ci_99_upper] = calculate_confidence_interval(avg_response_times[index], 0.99);

        csv_file << "," << ci_90_lower << "," << ci_90_upper << "," << ci_95_lower << "," << ci_95_upper << "," << ci_99_lower << "," << ci_99_upper;
        csv_file << std::endl;
    }
    return 0;
}