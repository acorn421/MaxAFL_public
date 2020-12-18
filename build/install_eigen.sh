#!/bin/bash
set -eux

LINUX_VER=${LINUX_VER:-ubuntu-18.04}
EIGEN_VER=${BOOST_VER:-3}

sudo apt-get install libeigen${EIGEN_VER}-dev -y


echo "Installing eigen is finished!!"
