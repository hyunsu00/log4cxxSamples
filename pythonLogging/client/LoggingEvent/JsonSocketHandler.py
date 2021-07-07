import logging
import logging.handlers
import socket

JSON_START_FLAG = b"\xAC\xED\x03\x05"

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
            clientSocket.send(JSON_START_FLAG)
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord -> json 바이트 전송
    def makePickle(self, record : logging.LogRecord) -> bytes:
        return super().makePickle(record)
