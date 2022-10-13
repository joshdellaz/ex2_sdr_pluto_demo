import socket
import sys
import threading

mtu = 10000
pdu = 128

tx_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tx_server_address = ('localhost', 1234)
tx_sock.connect(tx_server_address)

addr = input('Enter destination node address')

while True:
	tx_sock.send((0x44).to_bytes(1, 'big'))	


