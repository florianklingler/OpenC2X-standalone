#!/bin/sh
rm *.c *.h Makefile.am.sample
asn1c -fnative-types -gen-PER -pdu=auto its_facilities_pdu_all.asn    # ITS_CAM_v1.3.2.asn

find . -type l | xargs rm 

cp /usr/share/asn1c/*.c .
cp /usr/share/asn1c/*.h .
rm converter-sample.c
