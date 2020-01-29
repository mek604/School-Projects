#import socket module
from socket import *
import sys # In order to terminate the program

serverSocket = socket(AF_INET, SOCK_STREAM)


#Prepare a server socket
HOST = sys.argv[1]
PORT = int(sys.argv[2])
serverSocket.bind((HOST, PORT))
serverSocket.listen(2)

while True:
	#Establish the connection
	print('Ready to serve...')
	(connectionSocket, addr) =  serverSocket.accept()
            
	try:
		message = connectionSocket.recv(10000).decode()
		filename = message.split()[1]                 
		f = open(filename[1:]) 

		outputdata = f.read() 
		header = 'HTTP/1.1 200 OK\n\n'               
		#Send one HTTP header line into socket
		connectionSocket.send(header.encode())

		#Send the content of the requested file to the client
		for i in range(0, len(outputdata)):           
			connectionSocket.send(outputdata[i].encode())
		connectionSocket.send('\r\n'.encode())

		connectionSocket.close()
	except IOError:
    	#Send response message for file not found
		response = 'HTTP/1.1 404 Not Found'
		connectionSocket.send(response.encode())
		#Close client socket
		connectionSocket.close()

serverSocket.close()
sys.exit()#Terminate the program after sending the corresponding data
