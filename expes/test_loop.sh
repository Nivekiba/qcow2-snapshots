#!/bin/bash

num=1
while [ $num -le 10 ]; do
    echo $num
	num=$(($num+1))
    sleep 2
done &

pid=$!

sudo ./wss.pl $pid 1 -C

wait $pid

touch fisuu_$pid