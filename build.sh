#!/bin/bash

# ensure the target folder exists
mkdir -p ./build
cd ./build || exit
python ../assets2banks.py ../assets
sdcc -c -mz80 bank2.c --debug
sdcc -c -mz80 bank3.c --debug
sdcc -c -mz80 ../main.c --debug
sdcc -o cinos2.ihx -mz80 --data-loc 0xC000 --no-std-crt0 ../crt0_sms.rel main.rel SMSlib.lib bank2.rel bank3.rel --debug
ihx2sms cinos2.ihx cinos2.sms
