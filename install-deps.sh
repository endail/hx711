#!/bin/bash

# This script will automatically obtain and install
# the lgpio library if it is not already

if [ `id -u` -ne 0 ]; then
    echo "Run as root"
    exit 1;
fi

# build and install liblgpio if not found
if ! $(ldconfig -p | grep -q liblgpio); then

    # not cur dir and create tmp dir
    CURDIR=$(pwd)
    TMPDIR=$(mktemp -d)

    # cd into tmp dir and download lgpio
    cd "$TMPDIR"
    wget https://abyz.me.uk/lg/lg.zip
    unzip lg.zip

    # move into decompressed lgpio dir
    # build and install
    cd lg
    make
    make install

    # return to previous dir and remove
    # tmp files
    cd "$CURDIR"
    rm -rf "$TMPDIR"

else
    echo "liblgpio already installed"
fi
