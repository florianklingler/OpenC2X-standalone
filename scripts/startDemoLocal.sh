#!/bin/bash
SESSION=ETSI

#Runs all application locally in one terminal window tiled with tmux.
#If the connection to the tmux session is lost, reconnect via executing "tmux attach".
#To kill all applications execute "tmux kill-session" on the device

#navigating:
#press "Ctrl + b" and then a arrow key to change the focus to a panel in the corresponding direction
#resizing:
#press "Ctrl + b" then "Alt + Arrow key" to resize a panel in that direction
#new panel:
#press "Ctrl + b" then " " " or rather "Shift + 2" to  split a panel


tmux -2 new-session -d -s $SESSION


tmux set-option -g mouse

tmux new-window -t $SESSION:1 -n 'App'


tmux split-window -h
tmux select-pane -t 0

for APP in httpServer cam 
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

for APP in denm
do
    tmux send-keys "cd ../../build/binaries/$APP/bin" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
done 

tmux send-keys "cd ../../build/binaries/dcc/bin" C-m
tmux send-keys "sudo ./dcc" C-m
tmux split-window -v

tmux send-keys "cd ../../build/binaries/obd2/bin" C-m
tmux send-keys "./obd2" C-m
tmux split-window -v

tmux send-keys "cd ../../build/binaries/gps/bin" C-m
tmux send-keys "./gpsService" C-m

tmux -2 attach-session -t $SESSION

