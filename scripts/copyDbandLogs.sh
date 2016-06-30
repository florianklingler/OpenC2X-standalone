#!/bin/bash

if [ "$#" -ne 2 ]
then
  echo "Usage: ./copyDbandLogs.sh [box] [experiment number]"
  exit 1
fi

BOX=$1
EXP=$2

echo "Copying logs and db for experiment $EXP from $BOX"

mkdir ../results
scp $BOX:~/pg2015w/logs/$EXP_*.log ../results
scp $BOX:~/pg2015w/ldm/db/*-$EXP.db ../results
