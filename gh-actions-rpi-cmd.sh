
# note that this is running on a virtual rpi
export VIRTUAL_PI=1

apt-get update -y && apt-get full-upgrade -y
apt-get install git-all python3-dev python3-pip -y

./install-deps.sh
make && make install
