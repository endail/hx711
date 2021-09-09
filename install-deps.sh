#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Run as root"
    exit 1;
fi

# build and install liblgpio if not found
if ! $(ldconfig -p | grep -q liblgpio); then
    wget https://abyz.me.uk/lg/lg.zip
    unzip lg.zip
    cd lg
    make
    make install
    cd ..
else
    echo "liblgpio already installed"
fi
