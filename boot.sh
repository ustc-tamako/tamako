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
    qemu-system-i386 -S -s -hda tamako.img
else
    qemu-system-i386 -hda tamako.img
fi
exit 0