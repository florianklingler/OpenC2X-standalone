#!/bin/bash


if [ "$#" -ne 1 ]
then
  echo "Usage: ./sshTunnel [ssh config name]"
  exit 1
fi

echo "Stating SSH Tunnel to $1"
ssh -L 1188:localhost:1188 "$1" -N
