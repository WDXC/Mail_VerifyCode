#!/bin/bash
./configure --prefix=$PWD/install
make clean
make 
make install
cp ./install/bin/mail ./
make clean
