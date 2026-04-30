#!/bin/bash

echo "ArrivalRate,device_write,cpus,memory,cpuLoad,size,Throughput,ResponseTime" > loadtest_optimized.csv

for device_write in "5mb" "10mb" "20mb"
do
  for cpu in 0.5 1 2
  do
    for mem in '256m' '512m' '1g'
    do
      for arr in 10 20 40 75 100
      do
        for size in 0 250 1000
        do
          for cpuLoad in 0 100 1000
          do
            curl -X GET "http://10.130.152.55:8080/dockerStart.php?memory=${mem}&cpus=${cpu}&device-write=${device_write}" 2>/dev/null
            
            sleep 2
            
            httperf --client=0/1 \
                    --server=10.130.152.55 \
                    --port=10000 \
                    --uri="/server.php?size=${size}&cpuLoad=${cpuLoad}" \
                    --send-buffer=4096 \
                    --recv-buffer=16384 \
                    --num-conns=$((arr * 5)) \
                    --rate=${arr} > result.txt 2>&1
            
            awk -v var0="$arr" \
                -v var1="$device_write" \
                -v var2="$cpu" \
                -v var3="$mem" \
                -v var4="$cpuLoad" \
                -v var5="$size" \
                '{
                  OFS=","; 
                  if ($1 == "Reply" && $2 == "rate") t=$7; 
                  if ($1 == "Reply" && $2 == "time") r1 = $5;
                } 
                END {
                  print var0,var1,var2,var3,var4,var5,t,r1
                }' result.txt >> loadtest_optimized.csv
            
            curl -X GET "http://10.130.152.55:8080/dockerStop.php" 2>/dev/null
            
            sleep 1
          done
        done
      done                
    done    
  done
done