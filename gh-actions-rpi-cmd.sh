
# note that this is running on a virtual rpi
export VIRTUAL_PI=1
apt-get update -y && apt-get full-upgrade -y
ldconfig
./install-deps.sh
make && make install
ldconfig
