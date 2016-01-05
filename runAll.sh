#starts the applications in seperate gnome-terminals
#for persistent terminals use -nonClosing flag

nonClosing=""

if [ $# = 1 ] ;  then #check if one argument is supplied
    if [ $1 = "-nonClosing" ] ; then #if argument is nonClosing set text for commands 
        nonClosing=";bash"
    fi
fi

 
cd cam/Debug
gnome-terminal -e "bash -c './cam${nonClosing} | tee >(grep --line-buffer 'INFO' > ../../logs/debug_CaService.log) >(grep --line-buffered 'STATS' > ../../logs/stats_CaService.csv) >(grep -v --line-buffered -e 'INFO' -e 'STATS') > /dev/null'" &
cd ../..
 
cd dcc/Debug
gnome-terminal -e "bash -c './dcc${nonClosing} | tee >(grep --line-buffer 'INFO' > ../../logs/debug_Dcc.log) >(grep --line-buffered 'STATS' > ../../logs/stats_Dcc.csv) >(grep -v --line-buffered -e 'INFO' -e 'STATS') > /dev/null'" &
cd ../..

cd ldm/Debug
gnome-terminal -e "bash -c './ldm${nonClosing} | tee >(grep --line-buffer 'INFO' > ../../logs/debug_Ldm.log) >(grep --line-buffered 'STATS' > ../../logs/stats_Ldm.csv) >(grep -v --line-buffered -e 'INFO' -e 'STATS') > /dev/null'" &
cd ../..

cd denm/Debug
gnome-terminal -e "bash -c './denm${nonClosing} | tee >(grep --line-buffer 'INFO' > ../../logs/debug_DenService.log) >(grep --line-buffered 'STATS' > ../../logs/stats_DenService.csv) >(grep -v --line-buffered -e 'INFO' -e 'STATS') > /dev/null'" &
cd ../..

 
 
 
