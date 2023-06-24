#!/bin/python3

from scapy.all import *

localhost = "127.0.0.1"

s = socket.socket()
s.connect((localhost, 80))
ss = StreamSocket(s, Raw)
print(ss.sr1(Raw("M2")))