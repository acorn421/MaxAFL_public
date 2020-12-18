# !/bin/bash
# *install test targets

# *current target lists
# jhead-3.02
# tiff-4.0.9


# directory setup
mkdir /home/$USER/work/targets
cd /home/$USER/work/targets

# jhead
wget https://www.sentex.ca/~mwandel/jhead/jhead-3.02.tar.gz
tar -xzf jhead-3.02.tar.gz

# libtiff
wget https://download.osgeo.org/libtiff/tiff-4.0.9.tar.gz
tar -xzf tiff-4.0.9.tar.gz