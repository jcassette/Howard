#!/bin/bash

# decoller 
./clientudp 192.168.1.1 5556 AT*REF=101,290718208
sleep 5 

#monter
for i in {102..110}
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,1065353216,0 
sleep 0.03
done

#tourner dans un sens
for i in {111..210};
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,0,-1082130432 
sleep 0.03
done

#on considere le signal comme repréré : orientation
for i in {211..235};
do
./clientudp 192.168.1.1 5556  AT*PCMD=$i,0,0,0,0,1082130432 
sleep 0.03
done

#avancer
for i in {236..300};
do
./clientudp 192.168.1.1 5556 AT*PCMD=$i,1,0,-1090519040,0,0
sleep 0.03
done

#freiner
for i in {301..306};
do
./clientudp 192.168.1.1 5556 AT*PCMD=$i,1,0,1065353216,0,0
sleep 0.03
done


#aterrir
for i in {307..317};
do
./clientudp 192.168.1.1 5556  AT*REF=$i,290717696
done
