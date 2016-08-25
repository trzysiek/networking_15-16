from socket import *

serverName = 'localhost'
serverPort = 10000
clientSocket = socket(AF_INET, SOCK_DGRAM)
message = 'TITLE'
l = clientSocket.sendto(message, (serverName, serverPort))
print "wyslalem bajtow: ", l
print "moj socket to ", clientSocket
#resp, serverAddress = clientSocket.recvfrom(2048)
#print resp, serverAddress
clientSocket.close()
