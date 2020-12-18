#!/bin/bash
set -eux

LINUX_VER=${LINUX_VER:-ubuntu-18.04}

sudo apt-get install gnuplot lcov -y

echo "Installing packages is finished!!"
