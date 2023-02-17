#!/bin/bash
# wget https://www.cse.iitk.ac.in/users/mainakc/2023Spring/traces.zip
# unzip traces.zip
make clean && make
declare -A tracefiles
tracefiles["gromacs"]="1"
tracefiles["hmmer"]="1"
tracefiles["h264ref"]="1"
tracefiles["bzip2"]="2"
tracefiles["gcc"]="2"
tracefiles["sphinx3"]="2"

for file in "${!tracefiles[@]}"; do
    echo $file
    ./bin/simulator --file ./traces/$file.log_l1misstrace ${tracefiles[$file]} --max_assoc --belady
    echo ""
done