#!/bin/bash

for file in ../benchmarks/*/*/*.qasm; do
    q_cnt=0

    while read line; do
        if [[ $line =~ qreg ]]; then
            line=${line#"qreg qubits["}
            line=${line%"];"}
            q_cnt=$line
            break
        fi
    done < $file

    sed -i "/qreg/acreg qubits[$q_cnt]" $file

    for i in $(seq 0 $(($q_cnt-1)) ); do
        echo "measure q[$i] -> c[$i];" >> $file
    done
done