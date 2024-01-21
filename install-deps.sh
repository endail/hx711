#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Run as root";
    exit 1;
fi

# install lgpio from apt if available
apt-get install -y liblgpio-dev;

# otherwise build and install liblgpio-dev if not found
if ! $(ldconfig -p | grep -q liblgpio); then
    wget https://github.com/joan2937/lg/archive/master.zip;
    unzip master.zip;
    cd lg-master;
    make;
    make install;
    ldconfig;
    cd ..;
fi
