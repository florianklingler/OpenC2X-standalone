#!/bin/bash
SESSION=ETSI
SSH_WAIT_TIME=1s # not sure whether needed but if running the app fails after connecting, try increasing this value

#Opens for every programm a seperate ssh connection to $arg1 and runs it there. Also opens the Monitor Website locally in forefox
#All ssh connections are displayed in seperate tmux panels.
#Panels may be resized by mouse dragging. Also the focus can be changed by mouse clicking.
#The applications stop running if the ssh connection is closed/lost.
#To close all panels and with them the ssh connections execute "tmux kill-session" in any terminal

if [ "$#" -ne 1 ]
then
  echo "Usage: ./startDemo.sh [ssh config name]"
  exit 1
fi

BOX=$1

ssh $BOX "./setup_interfaces.sh 192.168.0.240"

tmux -2 new-session -d -s $SESSION

tmux new-window -t $SESSION:1 -n 'App'

tmux set-option -g mouse

echo "Starting dcc"
tmux send-keys "ssh $BOX" C-m
sleep $SSH_WAIT_TIME
tmux send-keys "cd ~/build/binaries/dcc/bin/" C-m
tmux send-keys "sudo ./dcc" C-m
tmux split-window -v
tmux select-layout tiled

for APP in denm cam obd2 httpServer
do
    echo "Starting $APP"
    # C-m semms to equal to pressing enter -> line is executed
    tmux send-keys "ssh $BOX" C-m
    sleep $SSH_WAIT_TIME
    tmux send-keys "cd ~/build/binaries/$APP/bin/" C-m
    tmux send-keys "./$APP" C-m
    tmux split-window -v
    tmux select-layout tiled
done 

echo "Starting gps"
tmux send-keys "ssh $BOX" C-m
sleep $SSH_WAIT_TIME
tmux send-keys "~/build/binaries/gps/bin/" C-m
tmux send-keys "./gpsService" C-m
tmux split-window -v
tmux select-layout tiled

echo "Starting ldm"
tmux send-keys "ssh $BOX" C-m
sleep $SSH_WAIT_TIME
tmux send-keys "~/build/binaries/ldm/bin/" C-m
tmux send-keys "rm ../db/ldm-1.db" C-m
tmux send-keys "./ldm" C-m
tmux split-window -v
tmux select-layout tiled

echo "Stating SSH Tunnel"
tmux send-keys "ssh -L 1188:localhost:1188 $BOX -N" C-m

tmux split-window -v
tmux select-layout tiled


firefox "./../webSite/index.html"&

tmux -2 attach-session -t $SESSION
