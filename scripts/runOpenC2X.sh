#!/bin/bash
# This script assumes that instructions in OpenC2X/README.txt have been followed

SESSION=OpenC2X
CURR_DIR=$(pwd)
OPENC2X=$CURR_DIR/..
BUILD_DIR=$OPENC2X/build/
GLOBAL_CONFIG=$OPENC2X/common/config/config.xml

LOCAL_CONFIG_RELATIVE=config/config.xml
LOGGING_CONF=config/logging.conf
STATISTICS_CONF=config/statistics.conf

tmux -2 new-session -d -s $SESSION


tmux set-option -g mouse

tmux new-window -t $SESSION:1 -n 'App'

tmux split-window -h
tmux select-pane -t 0

tmux send-keys "cd $BUILD_DIR/cam/src" C-m
tmux send-keys "./cam $GLOBAL_CONFIG $OPENC2X/cam/$LOCAL_CONFIG_RELATIVE $OPENC2X/cam/$LOGGING_CONF $OPENC2X/cam/$STATISTICS_CONF" C-m
tmux split-window -v

tmux send-keys "cd $BUILD_DIR/httpServer/src" C-m
tmux send-keys "./httpServer $GLOBAL_CONFIG $OPENC2X/httpServer/$LOCAL_CONFIG_RELATIVE $OPENC2X/httpServer/$LOGGING_CONF $OPENC2X/httpServer/$STATISTICS_CONF" C-m
tmux split-window -v

tmux send-keys "cd $BUILD_DIR/ldm/src" C-m
tmux send-keys "rm ../db/ldm-*.db" C-m
tmux send-keys "./ldm $GLOBAL_CONFIG $OPENC2X/ldm/$LOGGING_CONF $OPENC2X/ldm/$STATISTICS_CONF" C-m
tmux split-window -v

tmux kill-pane
tmux select-pane -t 3

tmux send-keys "cd $BUILD_DIR/denm/src" C-m
tmux send-keys "./denm $GLOBAL_CONFIG $OPENC2X/denm/$LOGGING_CONF $OPENC2X/denm/$STATISTICS_CONF" C-m
tmux split-window -v

tmux send-keys "cd $BUILD_DIR/dcc/src" C-m
tmux send-keys "sudo ./dcc $GLOBAL_CONFIG $OPENC2X/dcc/$LOCAL_CONFIG_RELATIVE $OPENC2X/dcc/$LOGGING_CONF $OPENC2X/dcc/$STATISTICS_CONF" C-m
tmux split-window -v

tmux send-keys "cd $BUILD_DIR/obd2/src" C-m
tmux send-keys "./obd2 $GLOBAL_CONFIG $OPENC2X/obd2/$LOCAL_CONFIG_RELATIVE $OPENC2X/obd2/$LOGGING_CONF $OPENC2X/obd2/$STATISTICS_CONF" C-m
tmux split-window -v

tmux send-keys "cd $BUILD_DIR/gps/src" C-m
tmux send-keys "./gpsService $GLOBAL_CONFIG $OPENC2X/gps/$LOCAL_CONFIG_RELATIVE $OPENC2X/gps/$LOGGING_CONF $OPENC2X/gps/$STATISTICS_CONF" C-m

tmux -2 attach-session -t $SESSION
