#!/bin/bash

cd /home/felix/projects/tyn/lbuilds
./lbuild.sh build ./lbuilds/qui/alpha/qui-alpha.lbuild || exit

if [ "$1" = "run" ]; then
	cd /home/felix/projects/tyn/tyndur ;
	rm -r ./build/root-local/packages/qui || exit ;
	./build/scripts/lpt_install.sh /home/felix/projects/tyn/lbuilds/packages/qui-alpha.tar ./build/root-local/ || exit 
	./build/scripts/image_hd_syslinux || exit ;
	./run.sh ; 
fi