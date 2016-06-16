#!/bin/sh

sed -i 's/IGNORE_STATS true/IGNORE_STATS false/g' dcc/src/PktStatsCollector.cpp

cd common/Debug
make all

cd ..
cd ..
cd cam/Debug
make clean
make all

cd ..
cd ..
cd denm/Debug
make clean
make all

cd ..
cd ..
cd dcc/Debug
make clean
make all

cd ..
cd ..
cd ldm/Debug
make clean
make all

cd ..
cd ..
cd gps/Debug
make clean
make all

cd ..
cd ..
cd obd2/Debug
make clean
make all

cd ..
cd ..
cd ldmWebApplication/Debug
make clean
make all
