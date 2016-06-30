#!/bin/bash

if [ "$#" -ne 1 ]
then
  echo "Usage: ./cleanDbandLogs.sh [experiment number]"
  exit 1
fi

echo "Deleting logs and db for experiment "$1

cd ../logs
rm -f "$1"_*.log "$1"_*.csv
cd ../ldm/db/
rm *-"$1".db
