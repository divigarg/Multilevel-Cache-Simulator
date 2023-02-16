#!/bin/bash
wget https://www.cse.iitk.ac.in/users/mainakc/2023Spring/traces.zip
unzip traces.zip
make clean && make
declare -A tracefiles
tracefiles["bzip2"]="2"
tracefiles["gcc"]="2"
tracefiles["sphinx3"]="2"
tracefiles["gromacs"]="1"
tracefiles["hmmer"]="1"
tracefiles["h264ref"]="1"

mkdir output/part1
for file in "${!tracefiles[@]}"; do
    ./bin/simulator --file ./traces/$file.log_l1misstrace ${tracefiles[$file]} > output/part1/${file}.txt &
    disown -h %1
done

mkdir output/part2_lru
for file in "${!tracefiles[@]}"; do
    ./bin/simulator --file ./traces/$file.log_l1misstrace ${tracefiles[$file]} --max_assoc > output/part2_lru/${file}_fa_lru.txt &
    disown -h %1
done

mkdir output/part2_belady
for file in "${!tracefiles[@]}"; do
    ./bin/simulator --file ./traces/$file.log_l1misstrace ${tracefiles[$file]} --max_assoc --belady > output/part2_belady/${file}_fa_belady.txt &
    disown -h %1
done