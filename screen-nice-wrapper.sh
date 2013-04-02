#!/bin/bash

screen -S skein -d -m nice -n 15 ./submit.py
echo "Screen started; screen -ls to see what screens are open."
