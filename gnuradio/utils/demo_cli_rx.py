import socket
import sys
import threading

mtu = 10000

rx_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
rx_server_address = ('localhost',4321) 
rx_sock.connect(rx_server_address)

addr = input('Enter rx node address:')

while True:
	rx_data = rx_sock.recv(mtu)
	rx_data = ''.join(chr(i) for i in rx_data)
	rx_data = rx_data.split(':')
	if rx_data[0] == addr:
		print('Received data:')
		print(str(rx_data[1]))
