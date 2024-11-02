#!/bin/bash

dimx_start=4
dimx_end=32
dimy_start=4
dimy_end=32
buffer_sizes=(4 8 16)
packet_size_min=2
packet_size_max=20
routing_types=("FT_ODD_EVEN")
# selection_strategies=("random" "bufferlevel" "nop")  # Various selection strategies
traffic_types=("random" "transpose1" "transpose2" "bitreversal" "butterfly")  # More traffic types
simulation_time=$(seq 21000 1000 40000) # Short simulation time to speed up testing
fault_rate=$(seq 0.001 0.001 0.009)
pir=$(seq 0.001 0.001 0.009)

# Output CSV file for results
output_file="noxim_results.csv"

echo "simulation_time,dimx,dimy,buffer_size,routing_type,traffic_type,injection_rate,fault_rate,global_avg_delay,network_throughput,ip_avg_throughput,max_delay,total_energy" >"$output_file"

# Total sample count
sample_count=0

# Define the log file

run_command() {
  local argument=$1
  read -r inputs command <<< "$argument"
  # local inputs=$1
  # shift
  # local command="$*"

  echo "Running command: $command"
  echo "input job : $inputs"
  # Run the command, and redirect stdout and stderr to the log file
  # with flock to prevent conflict
  output=$(eval "$command")
  global_avg_delay=$(echo "$output" | grep "Global average delay (cycles):" | awk '{print $NF}')
  network_throughput=$(echo "$output" | grep "Network throughput (flits/cycle):" | awk '{print $NF}')
  ip_avg_throughput=$(echo "$output" | grep "Average IP throughput (flits/cycle/IP)" | awk '{print $NF}')
  max_delay=$(echo "$output" | grep "Max delay" | awk '{print $NF}')
  total_energy=$(echo "$output" | grep "Total energy" | awk '{print $NF}')
  # Store results in CSV format
  (
    flock -x 200
    echo "$inputs,$global_avg_delay,$network_throughput,$ip_avg_throughput,$max_delay,$total_energy" >>"$output_file"
  ) 200>"$output_file.lock"
}

commands=()

# Generate samples for powers of two for x and y dimensions
for ((dimx = dimx_start; dimx <= dimx_end; dimx *= 2)); do
  #for ((dimy = dimy_start; dimy <= dimy_end; dimy *= 2)); do
    for buffer in "${buffer_sizes[@]}"; do
      for injection_rate in $pir; do
        for routing in "${routing_types[@]}"; do
          for traffic in "${traffic_types[@]}"; do
            for fault in $fault_rate; do
              for sim_time in $simulation_time; do
              	sample=$((sample + 1))
                inputs="$sim_time,$dimx,$dimx,$buffer,$routing,$traffic,$injection_rate,$fault"
                commands+=("$sample,$inputs noxim -config default_config.yaml  -dimx $dimx -dimy $dimx -buffer $buffer -routing $routing -pir $injection_rate poisson -fault $fault -sim $sim_time -traffic $traffic")                            
              done
            done
          done
        done
      done
    done
  #done
done

echo "Samples count : ${#commands[@]}"

export -f run_command
export output_file

# for item in "${commands[@]}"; do    
#     run_command "$item"
# done
#parallel -j 12 run_command ::: "${commands[@]}"

#batch_size=128
#total_commands=${#commands[@]}

# for (( i=0; i<total_commands; i+=batch_size )); do
#     batch=("${commands[@]:i:batch_size}")
#     echo "New batch ${#batch[@]}"    
#     parallel --halt now,fail=1 run_command ::: "${batch[@]}"
# done


printf "%s\n" "${commands[@]}" > commands.txt

echo "Created commands.txt file"

#cat commands.txt | parallel -N 128 --halt now,fail=1 run_command
cat commands.txt | parallel -N 128 run_command
#printf "%s\n" "${commands[@]}" | parallel -N 128 --halt now,fail=1 run_command


# printf "%s\n" "${commands[@]}" | \
#     xargs -n 128 | \
#     while read -r batch; do
#         echo "$batch" #| parallel -j 6 run_command
#     done

# echo "Simulations complete"
