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
	to_send = input('Enter some text to send:')
	to_send = "$$$$" + addr + ":" + to_send
	to_send = to_send.encode('ascii')
	stuffing = (pdu - len(to_send))*' '
	to_send = to_send + stuffing.encode('ascii')
	tx_sock.send(to_send)
	


