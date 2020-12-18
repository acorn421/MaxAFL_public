#!/bin/bash
set -eux

LINUX_VER=${LINUX_VER:-ubuntu-18.04}
LIBLGFGS_VER=${LIBLGFGS_VER:-1.10}
PREFIX=${PREFIX:-${HOME}}

# LLVM_DEP_URL=http://releases.llvm.org/${LLVM_VER}
TAR_NAME=v${LIBLGFGS_VER}.tar.gz
DIR_NAME=liblbfgs-${LIBLGFGS_VER}

sudo apt install libtool automake -y

wget -q https://github.com/chokkan/liblbfgs/archive/${TAR_NAME}
tar -C ${PREFIX} -xf ${TAR_NAME}
rm ${TAR_NAME}
# mv ${PREFIX}/${TAR_NAME} ${PREFIX}/liblbfgs-${LIBLGFGS_VER}

cd ${PREFIX}/${DIR_NAME}
sudo ./autogen.sh
sudo ./configure
sudo make
sudo make install

set +x
echo "Installing liblbfgs is finished!!"
