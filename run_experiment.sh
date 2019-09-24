#!/bin/bash

SPEED=10
DELAY=50
LOSS=5
CORRUPT=20

FILES=(file1.bin file2.bin file3.bin)

killall link
killall kreceiver
killall ksender

./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT &> /dev/null &
sleep 1
./kreceiver &
sleep 1

./ksender "${FILES[@]}"

echo "==========================="
for file in "${FILES[@]}"
do
	DIFF=$(diff $file recv_$file) 
	if [ "$DIFF" != "" ] 
	then
		echo "Files $file and recv_$file are not the same!"
	fi
done
echo "==========================="
