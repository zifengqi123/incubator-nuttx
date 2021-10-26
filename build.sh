#! /bin/bash

echo "start to build elf all..."
make clean
make
make export
cd addon/
rm nuttx-export-10.1.0* -rf
mv ../nuttx-export-10.1.0.zip .
unzip nuttx-export-10.1.0.zip

cd ledcontrol/
make clean
make
cp ledcontrol ~/nfs_share/
cd ../..

echo "done."
