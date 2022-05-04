#!/bin/bash

make clean && make CROSS_COMPILE=x86_64-elf- && make mac_install