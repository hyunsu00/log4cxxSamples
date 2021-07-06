import logging
import socket

#####
# JsonSockeHandler
#####
class JsonSocketHandler(logging.handlers.SocketHandler):
    """description of class"""
    def __init__(self, host, port):
        super().__init__(host, port)
    
    # 연결시 시작 flag를 보낸다.
    def makeSocket(self, timeout=1):
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            clientSocket.connect(self.address)
            clientSocket.send(b"\xAC\xED\x03\x05")
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord -> json 바이트 전송
    def makePickle(self, record : logging.LogRecord) -> bytes:
        return super().makePickle(record)
