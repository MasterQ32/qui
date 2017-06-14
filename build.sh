#!/bin/bash

cd /home/felix/projects/tyn/lbuilds
./lbuild.sh build ./lbuilds/qui/alpha/qui-alpha.lbuild || exit

if [ "$1" = "run" ]; then
	cd /home/felix/projects/tyn/tyndur ;
	./build/scripts/image_hd_syslinux || exit ;
	./run.sh ; 
fi