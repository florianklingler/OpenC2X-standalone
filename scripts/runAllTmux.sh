#!/bin/bash
SESSION=ETSI
BOX=apu2

tmux -2 new-session -d -s $SESSION

tmux set-option -g mouse

tmux new-window -t $SESSION:1 -n 'App'


tmux split-window -h
tmux select-pane -t 0

for APP in ldm ldmWebApplication cam 
do
    tmux send-keys "ssh $BOX"
    tmux send-keys "sleep 1"
    tmux send-keys "cd pg2015w/scripts"
    tmux send-keys "cd ../$APP/Debug" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 

tmux kill-pane
tmux select-pane -t 3

for APP in gps obd2 denm
do
    tmux send-keys "ssh $BOX"
    tmux send-keys "sleep 1"
    tmux send-keys "cd pg2015w/scripts"
    tmux send-keys "cd ../$APP/Debug" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 

tmux send-keys "ssh $BOX"
tmux send-keys "sleep 1"
tmux send-keys "cd pg2015w/scripts"
tmux send-keys "cd ../dcc/Debug" C-m
tmux send-keys "sudo ./dcc" C-m



tmux -2 attach-session -t $SESSION