import socket
import sys
import threading

mtu = 10000

tx_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tx_server_address = ('localhost', 1234)
tx_sock.bind(tx_server_address)

addr = input('Enter destination node address')

while True:
	to_send = input('Enter some text to send:')
	tx_sock.send(addr + " " + to_send)
	


