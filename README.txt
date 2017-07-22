----------------------------------------------------------------------------------------------------
                                             OpenC2X
----------------------------------------------------------------------------------------------------
For latest updates on OpenC2X, visit http://www.ccs-labs.org/software/openc2x/

On Ubuntu 16.04, follow the below mentioned steps to get OpenC2X up and running.
For other OS users, we recommend you to setup a virtual box with Ubuntu 16.04.

OpenC2X source is organized in the tree structure like this:

path_to_openc2x
|
└── OpenC2X
    ├── CMakeLists.txt
    ├── cam
    ├── common
    ├── dcc
    ├── denm
    ├── denmApp
    ├── gps
    ├── httpServer
    ├── ldm
    ├── obd2
    └── kernel-patches


----------------------------------------------------------------------------------------------------
1)                                      Set up the environment
----------------------------------------------------------------------------------------------------
Open a terminal and run the command:

    $ sudo apt-get install libzmq3-dev libboost-all-dev protobuf-compiler libprotobuf-dev \
libgps-dev gpsd gpsd-clients libnl-3-200 libnl-3-dev libnl-genl-3-200 libnl-genl-3-dev sqlite3 \
libsqlite3-dev tmux asn1c build-essential cmake doxygen



----------------------------------------------------------------------------------------------------
2)                           Generate source from .asn before compiling
----------------------------------------------------------------------------------------------------
In the terminal, navigate to asn1 folder in common:

    $ cd /path_to_openc2x/OpenC2X/common/asn1/
    $ ./generate.sh



----------------------------------------------------------------------------------------------------
3)                                         Compiling
----------------------------------------------------------------------------------------------------
In the terminal, navigate to your project folder and configure the project:

    $ cd path_to_openc2x/OpenC2X/
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make all



----------------------------------------------------------------------------------------------------
4)                                      Configurations
----------------------------------------------------------------------------------------------------
- The configuration of the modules can be changed in 'OpenC2X/<module>/config/'
- Make sure to change the interface name and the stationId in 'common/config/config.xml'
  as per your setup.
- For setting up OpenC2X kernel, go through 'kernel-patches/README.txt'



----------------------------------------------------------------------------------------------------
5)                                        Start OpenC2X
----------------------------------------------------------------------------------------------------
If you have followed the above steps, then you are ready to run OpenC2X.
In the terminal, navigate to scripts:

    $ cd path_to_openc2x/OpenC2X/scripts/
    $ ./runOpenC2X.sh

For the GUI, open 'webSite/index.html' in your browser. You can also trigger denms from there.
You can stop your experiment by running stopOpenC2X.sh in another terminal.
