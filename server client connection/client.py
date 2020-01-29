from socket import *
import sys


address = sys.argv[1]
port = int(sys.argv[2])
f = sys.argv[3]

s = socket(AF_INET, SOCK_STREAM)
s.connect((address, port))
get = 'GET /' + f + ' HTTP/1.1\r\n\r\n'
s.send(get.encode())

data = s.recv(10000).decode()
	

"""HOST = sys.argv[1]
PORT = int(sys.argv[2])
f = sys.argv[3]

get = 'GET /' + f + ' HTTP/1.1\r\n\r\n'
with socket(AF_INET, SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.send(get.encode())
    data = s.recv(1024).decode()
"""
print(data)

s.close()
