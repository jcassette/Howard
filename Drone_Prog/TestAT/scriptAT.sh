#!/bin/bash

#calibrer
./clientudp 192.168.1.1 5556 AT*FTRIM=100,1
sleep 1

# decoller 
./clientudp 192.168.1.1 5556 AT*REF=101,290718208
sleep 5 

#monter
for i in {102..152}
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,1065353216,0 
sleep 0.03
done

#tourner dans un sens
for i in {153..453};
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,0,-1090519040
sleep 0.03
done

#on considere le signal comme repréré : orientation
for i in {454..554};
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,0,1056964608
sleep 0.03
done

#avancer
for i in {554..604};
do
./clientudp 192.168.1.1 5556 AT*PCMD=$i,1,0,-1090519040,0,0
sleep 0.03
done

#freiner
for i in {605..610};
do
./clientudp 192.168.1.1 5556 AT*PCMD=$i,1,0,1056964608,0,0
sleep 0.03
done

sleep 2

#aterrir
for i in {611..615};
do
./clientudp 192.168.1.1 5556  AT*REF=$i,290717696
done
