rm -f cpuLoad.log
while true; do uptime >> cpuLoad.log; sleep 1; done
