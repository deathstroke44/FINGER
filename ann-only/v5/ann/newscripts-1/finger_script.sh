#!/bin/bash
#SBATCH -t 40:00:00
#SBATCH -A PAS2671
data=[data]
M=[M]
EFS=[EFS]
EFC=[EFC]

/usr/bin/time -v ./go /users/PAS2671/kabir36/kabir/similarity-search/dataset/${data} /users/PAS2671/kabir36/kabir/similarity-search/dataset/${data} l2 ${M} ${EFC} 1 ${EFS} 0 0 1 0 0 &> logs/${data}-${M}-${EFS}.log
