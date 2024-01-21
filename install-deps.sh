#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Run as root";
    exit 1;
fi

#ldconfig -p;
#ldconfig;
#ldconfig -p;

apt-get install -y liblgpio-dev;

# build and install liblgpio if not found
#if ! $(ldconfig -p | grep -q liblgpio); then
#    wget https://github.com/joan2937/lg/archive/master.zip;
#    unzip master.zip;
#    cd lg-master;
#    make;
#    make install;
#    ldconfig;
#    cd ..;
#else
#    echo "liblgpio already installed";
#fi
