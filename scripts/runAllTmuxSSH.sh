#!/bin/bash
SESSION=ETSI
BOX=gericom1
SSH_WAIT_TIME=1

echo "Starting"

tmux -2 new-session -d -s $SESSION


tmux new-window -t $SESSION:1 -n 'App'

tmux set-option -g mouse



tmux split-window -h
tmux select-pane -t 0

for APP in ldm ldmWebApplication cam 
do
    echo "Starting $APP"
    # C-m semms to equal to pressing enter -> line is executed
    tmux send-keys "ssh $BOX" C-m
    sleep $SSH_WAIT_TIME
    tmux send-keys "cd pg2015w/scripts" C-m
    tmux send-keys "cd ../$APP/Debug" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 

tmux kill-pane
tmux select-pane -t 3

for APP in gps obd2 denm
do
    echo "Starting $APP"
    tmux send-keys "ssh $BOX" C-m
    sleep $SSH_WAIT_TIME
    tmux send-keys "cd pg2015w/scripts" C-m
    tmux send-keys "cd ../$APP/Debug" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 


echo "Starting dcc"
tmux send-keys "ssh $BOX" C-m
sleep $SSH_WAIT_TIME
tmux send-keys "cd pg2015w/scripts" C-m
tmux send-keys "cd ../dcc/Debug" C-m
tmux send-keys "sudo ./dcc" C-m


tmux -2 attach-session -t $SESSION
