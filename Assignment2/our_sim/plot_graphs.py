import matplotlib.pyplot as py
import numpy as np

# Read from input_file (which is a csv) and has columns
# user_count,avg_response_time,good_throughput,bad_throughput,total_throughput,drop_percentage,server_0_utilization,ci_90_lower,ci_90_upper,ci_95_lower,ci_95_upper,ci_99_lower,ci_99_upper
# And plot the graphs for avg_response_time, total_throughput, drop_percentage, server_0_utilization with respect to user_count
def plot_graphs(input_file):
    # Get number of servers from the header of the csv file
    with open(input_file, 'r') as f:
        header = f.readline().strip().split(',')
        num_servers = len(header) - 12  # Subtract the first 12 columns
    data = np.genfromtxt(input_file, delimiter=',', skip_header=1)
    user_count = data[:, 0]
    avg_response_time = data[:, 1]
    total_throughput = data[:, 4]
    drop_percentage = data[:, 5]

    # Plot avg_response_time vs user_count
    py.figure(figsize=(10, 6))
    py.plot(user_count, avg_response_time, marker='o')
    py.title('Average Response Time vs User Count')
    py.xlabel('User Count')
    py.ylabel('Average Response Time (ms)')
    py.grid()
    py.savefig('avg_response_time.png')

    # Plot total_throughput vs user_count
    py.figure(figsize=(10, 6))
    py.plot(user_count, total_throughput, marker='o')
    py.title('Total Throughput vs User Count')
    py.xlabel('User Count')
    py.ylabel('Total Throughput (requests/sec)')
    py.grid()
    py.savefig('total_throughput.png')

    # Plot drop_percentage vs user_count
    py.figure(figsize=(10, 6))
    py.plot(user_count, drop_percentage, marker='o')
    py.title('Drop Percentage vs User Count')
    py.xlabel('User Count')
    py.ylabel('Drop Percentage (%)')
    py.grid()
    py.savefig('drop_percentage.png')

    # Iterate through each server and plot server_utilization vs user_count
    for i in range(num_servers):
        server_utilization = data[:, 6 + i]
        py.figure(figsize=(10, 6))
        py.plot(user_count, server_utilization, marker='o')
        py.title(f'Server {i} Utilization vs User Count')
        py.xlabel('User Count')
        py.ylabel(f'Server {i} Utilization (%)')
        py.grid()
        py.savefig(f'server_{i}_utilization.png')

if __name__ == "__main__":
    input_file = 'withoutresend.csv'  # Change this to your actual input file
    plot_graphs(input_file)