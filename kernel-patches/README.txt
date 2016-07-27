For the setting up the openC2X kernel, following needs to be done.

$ git clone https://github.com/CTU-IIG/802.11p-linux.git
$ cd 802.11p-linux
$ git checkout its-g5_v3
$ git am < 0001-Enable-queueing-in-all-4-ACs-BE-BK-VI-VO.patch
$ git am < 0002-Get-hw-queue-pending-stats-from-ath9k-via-netlink.patch

Build the kernel and have fun with openC2X
