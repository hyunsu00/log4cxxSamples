import logging
import logging.handlers
import socket
import msgpack

MSGPACK_START_FLAG = b"\xAC\xED\x02\x05"

#####
# MsgpackSocketHandler
#####
class MsgpackSocketHandler(logging.handlers.SocketHandler):
    """description of class"""
    def __init__(self, host, port):
        super().__init__(host, port)

    # 연결시 시작 flag를 보낸다.
    def makeSocket(self, timeout=1):
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            clientSocket.connect(self.address)
            clientSocket.send(MSGPACK_START_FLAG)
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord -> msgpack 바이트 전송
    def makePickle(self, record : logging.LogRecord) -> bytes:
        data = msgpack.packb(record.__dict__)
        # 현재는 보내지 않음
        # 보낼려면 return dataLen + data
        # dataLen = struct.pack("!L", len(data))
        return data
