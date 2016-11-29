#!/bin/bash
# This script assumes that OpenC2X source is organised in the tree structure like this:
#.
#├── build
#└── src
#    ├── CMakeLists.txt
#    ├── cam
#    ├── common
#    ├── dcc
#    ├── denm
#    ├── denmApp
#    ├── gps
#    ├── httpServer
#    ├── ldm
#    ├── obd2
#    └── kernel-patches

tmux -2 new-session -d -s $SESSION


tmux set-option -g mouse

tmux new-window -t $SESSION:1 -n 'App'

tmux split-window -h
tmux select-pane -t 0

for APP in cam httpServer
do
    tmux send-keys "cd ../../build/binaries/$APP/bin" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 

tmux send-keys "cd ../../build/binaries/ldm/bin" C-m
tmux send-keys "rm ../db/ldm-1.db" C-m
tmux send-keys "./ldm" C-m
tmux split-window -v

tmux kill-pane
tmux select-pane -t 3

tmux send-keys "cd ../../build/binaries/denm/bin" C-m
tmux send-keys "./denm" C-m
tmux split-window -v

tmux send-keys "cd ../../build/binaries/dcc/bin" C-m
tmux send-keys "sudo ./dcc" C-m
tmux split-window -v

tmux send-keys "cd ../../build/binaries/obd2/bin" C-m
tmux send-keys "./obd2" C-m
tmux split-window -v

tmux send-keys "cd ../../build/binaries/gps/bin" C-m
tmux send-keys "./gpsService" C-m

tmux -2 attach-session -t $SESSION
