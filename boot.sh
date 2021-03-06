#!/bin/bash

qemu=1
bochs=0
debug=0

while getopts "qbd" arg
do
	case $arg in
		q)  qemu=1
		;;
		b)  bochs=1
		;;
		d)  debug=1
		;;
	esac
done

if [ $bochs -eq 1 ]
then
	if [ $debug -eq 1 ]
	then
		bochs-gdb -f tools/.dbochsrc
	else
		bochs -f tools/.bochsrc
	fi
	exit 0
fi
	
if [ $debug -eq 1 ]
then
	qemu-system-i386 -m 32M -S -s -hda tamako.img --nographic
else
	qemu-system-i386 -m 32M -hda tamako.img --nographic
fi
exit 0

# cgdb -d x86_64-elf-gdb -ex "target remote :1234" tamago