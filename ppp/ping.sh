#!/bin/bash
while true
do
ping -c 1 baidu.com &>/dev/null
if [ $? -eq 0 ];then
echo "Ip is up "
else
echo "ip is down"
fi
sleep 1s
done 
