import logging
import logging.handlers
import socket
import struct

BYTE_START_FLAG = b"\xAC\xED\x01\x05"

#####
# ByteSocketHandler
#####
class ByteSocketHandler(logging.handlers.SocketHandler):
    """description of class"""
    def __init__(self, host, port):
        super().__init__(host, port)

    # 연결시 시작 flag를 보낸다.
    def makeSocket(self, timeout=1):
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            clientSocket.connect(self.address)
            clientSocket.send(BYTE_START_FLAG)
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord 필요한 데이터 바이트 전송
    def makePickle(self, record : logging.LogRecord) -> bytes:
        # len | logger | int          | len | message | log4cxx_time_t | len | threadName | len | fileName | len | methodName | len | lineNumber
        #   name       | int(levelno) |    msg        | int(created)   |  threadName      |  pathname      |  funcName        |  str(lineno)

        nameLen = len(record.name.encode()); name = record.name.encode()
        msgLen = len(record.msg.encode()); msg = record.msg.encode()
        threadNameLen = len(record.threadName.encode()); threadName = record.threadName.encode()
        pathnameLen = len(record.pathname.encode()); pathname = record.pathname.encode()
        funcNameLen = len(record.funcName.encode()); funcName = record.funcName.encode()
        linenoLen = len(str(record.lineno).encode()); lineno = str(record.lineno).encode()
        data = struct.pack(
            "! I %ds i I %ds Q I %ds I %ds I %ds I %ds" % (nameLen, msgLen, threadNameLen, pathnameLen, funcNameLen, linenoLen),
            nameLen, name,
            int(record.levelno) * 1000,
            msgLen, msg,
            round(record.created * 1000 * 1000),
            threadNameLen, threadName,
            pathnameLen, pathname,
            funcNameLen, funcName,
            linenoLen, lineno,
        )
        dataLen = len(data)
        self.debugWrite("record.bin", data)

        # 현재는 보내지 않음
        # 보낼려면 return dataLen + data
        # dataLen = struct.pack("!L", dataLen)
        return data
