#!/bin/bash

cd ./build || exit
python2.7 ../assets2banks.py ../assets
sdcc -c -mz80 bank2.c --debug
sdcc -c -mz80 ../map/map1_l1.c --debug
sdcc -c -mz80 ../main.c --debug
sdcc -o cinos2.ihx -mz80 --data-loc 0xC000 --no-std-crt0 ../crt0_sms.rel main.rel SMSlib.lib bank2.rel map1_l1.rel --debug
ihx2sms cinos2.ihx cinos2.sms
