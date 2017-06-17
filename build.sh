#!/bin/bash

cd /home/felix/projects/tyn/lbuilds
./lbuild.sh build ./lbuilds/qui/0.1/qui-0.1.lbuild || exit

if [ "$1" = "install" ]; then
	cd /home/felix/projects/tyn/tyndur ;
	rm -r ./build/root-local/packages/qui || exit ;
	./build/scripts/lpt_install.sh /home/felix/projects/tyn/lbuilds/packages/qui-0.1.tar ./build/root-local/ || exit ;
fi

if [ "$1" = "run" ]; then
	cd /home/felix/projects/tyn/tyndur ;
	rm -r ./build/root-local/packages/qui || exit ;
	./build/scripts/lpt_install.sh /home/felix/projects/tyn/lbuilds/packages/qui-0.1.tar ./build/root-local/ || exit ;
	./build/scripts/image_hd_syslinux || exit ;
	./run.sh ; 
fi