#!/bin/bash
SESSION=ETSI
BOX=gericom1
SSH_WAIT_TIME=0.2s # not sure whether needed but if running the app fails after connecting, try increasing this value

#Opens for every programm a seperate ssh connection to $BOX and runs it there.
#All ssh connections are displayed in seperate tmux panels.
#Panels may be resized by mouse dragging. Also the focus can be changed by mouse clicking.
#The applications stop running if the ssh connection is closed/lost.
#To close all panels and with them the ssh connections execute "tmux kill-session" in any terminal

echo "Starting"

tmux -2 new-session -d -s $SESSION


tmux new-window -t $SESSION:1 -n 'App'

tmux set-option -g mouse


for APP in ldm ldmWebApplication cam gps obd2 denm
do
    echo "Starting $APP"
    # C-m semms to equal to pressing enter -> line is executed
    tmux send-keys "ssh $BOX" C-m
    sleep $SSH_WAIT_TIME
    tmux send-keys "cd pg2015w/scripts" C-m
    tmux send-keys "cd ../$APP/Debug" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
    tmux select-layout tiled
done 

echo "Stating SSH Tunnel"
tmux send-keys "ssh -L 1188:localhost:1188 $BOX -N" C-m

tmux split-window -v
tmux select-layout tiled

echo "Starting dcc"
tmux send-keys "ssh $BOX" C-m
sleep $SSH_WAIT_TIME
tmux send-keys "cd pg2015w/scripts" C-m
tmux send-keys "cd ../dcc/Debug" C-m
tmux send-keys "sudo ./dcc" C-m


firefox "./../ldmWebSite/index.html"&

tmux -2 attach-session -t $SESSION
