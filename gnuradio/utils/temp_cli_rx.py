import socket
import sys
import threading

mtu = 10000

tx_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tx_server_address = ('localhost', 1234)
tx_sock.bind(tx_server_address)


rx_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
rx_server_address = ('localhost',4321) 
rx_sock.bind(rx_server_address)
rx_sock.setblocking(False)

class InputThread(threading.Thread):
	    def __init__(self, input_cbk = None, name='keyboard-input-thread'):
        self.input_cbk = input_cbk
        super(KeyboardThread, self).__init__(name=name)
        self.start()

    def run(self):
        while True:
            self.input_cbk(input()) #waits to get input + Return

def my_callback(inp):
    #evaluate the keyboard input
    print('You Entered:', inp, ' Counter is at:', showcounter)

#start the Keyboard thread
kthread = KeyboardThread(my_callback)

while True:
	to_send = input('Enter some text to send:)
	if to_send:
		tx_sock.send(to_send)
	rx_data = rx_sock.recv(mtu)
	if rx_data > 0 
		print('Received data:')
		print(rx_data)
	


