#!/bin/bash
mpiexec -n 4 --hostfile hosts.txt --map-by node ./MPI_radixsort input.txt 8 0 8
mpiexec -n 4 --hostfile host-titan.txt --map-by node ./MPI_radixsort input.txt 1200000 0 100
