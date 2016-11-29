For setting up openC2X kernel with ath9k chipset, following needs to be done:

$ git clone https://github.com/CTU-IIG/802.11p-linux.git
$ cd 802.11p-linux
$ git checkout its-g5_v3
$ git am < 0001-Enable-queueing-in-all-4-ACs-BE-BK-VI-VO.patch
$ git am < 0002-Get-hw-queue-pending-stats-from-ath9k-via-netlink.patch

Build the kernel and have fun with openC2X!


----------------------------------------------------------------------------------------------------
                                      To compile kernel
----------------------------------------------------------------------------------------------------
$ mkdir _build_i386
$ make O=_build_i386 i386_defconfig
$ cd _build_i386
$ make menuconfig # important: select wireless card drivers (atheros) and other necessary drivers
$ make deb-pkg LOCALVERSION=-openc2x KDEB_PKGVERSION=1 -j4 # replace 4 with number of cpu cores you have
