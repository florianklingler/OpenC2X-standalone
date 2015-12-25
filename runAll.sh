#starts the applications in seperate gnome-terminals
#for persistent terminals use -nonClosing flag

nonClosing=""

if [ $# = 1 ] ;  then #check if one argument is supplied
    if [ $1 = "-nonClosing" ] ; then #if argument is nonClosing set text for commands 
        nonClosing=";bash"
    fi
fi

 
cd cam/Debug
gnome-terminal -e "bash -c ./cam${nonClosing}" &
cd ../..
 
cd dcc/Debug
gnome-terminal -e "bash -c ./dcc${nonClosing}" &
cd ../..

cd ldm/Debug
gnome-terminal -e "bash -c ./ldm${nonClosing}" &
cd ../..

cd denm/Debug
gnome-terminal -e "bash -c ./denm${nonClosing}" &
cd ../..

 
 
 