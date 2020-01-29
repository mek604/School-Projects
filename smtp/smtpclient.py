from socket import *
from sys import *

msg = "\r\n Hello it is me!"
endmsg = "\r\n.\r\n"

# Choose a mail server (e.g. Google mail server) and call it mailserver
# mailserver = ?????????

if(len(argv) == 2):
	mailserver = argv[1]
	port = argv[2]
else:
	mailserver = 'smtp.uvic.ca'
	port = 25
# Create socket called clientSocket and establish a TCP connection with mailserver
# ???????
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((mailserver, port))


recv = clientSocket.recv(1024).decode()
print(recv)
if recv[:3] != '220':
	print('220 reply not received from server.')

# Send HELO command and print server response.
heloCommand = 'HELO Alice\r\n'
clientSocket.send(heloCommand.encode())
recv1 = clientSocket.recv(1024).decode()
print(recv1)
if recv1[:3] != '250':
    print('250 reply not received from server.')
    
# Send MAIL FROM command and print server response.
# ??????
mailFrom = "MAIL FROM:<aobchey00@uvic.ca>\r\n"
clientSocket.send(mailFrom.encode())
recv2 = clientSocket.recv(1024).decode()
print("Server reply: %s\n" % (recv2))


# Send RCPT TO command and print server response. 
# ??????
rcptTo = "RCPT TO:<aobchey00@uvic.ca>\r\n"
clientSocket.send(rcptTo.encode())
recv3 = clientSocket.recv(1024).decode()
print("Server reply: %s\n" % (recv3))


# Send DATA command and print server response. 
# ??????
data = "DATA\r\n"
clientSocket.send(data.encode())
recv4 = clientSocket.recv(1024).decode()
print("Server reply: %s\n" % (recv4))

# Send message data.
# ??????
clientSocket.send(msg.encode())

# Message ends with a single period.
# ???????
clientSocket.send(endmsg.encode())
recv_msg = clientSocket.recv(1024)
print("Server reply: %s\n" % (recv_msg.decode()))

# Send QUIT command and get server response.
# ???????
quit = "QUIT\r\n"
clientSocket.send(quit.encode())
recv5 = clientSocket.recv(1024).decode()
print("Server reply: %s\n" % (recv5))
clientSocket.close()