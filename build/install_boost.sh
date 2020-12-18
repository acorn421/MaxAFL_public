#!/bin/bash
set -eux

LINUX_VER=${LINUX_VER:-ubuntu-18.04}
BOOST_VER=${BOOST_VER:-1.62}
PREFIX=${PREFIX:-${HOME}}

sudo apt-get install libboost-graph${BOOST_VER}-dev -y
sudo apt-get install libboost-serialization${BOOST_VER}-dev -y


echo "Installing boost is finished!!"