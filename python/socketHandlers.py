# socketHandlers.py
import logging.handlers
import socket
import struct
import msgpack

#####
# bytesSockethandler
#####
class bytesSocketHandler(logging.handlers.SocketHandler):
    def __init__(self, host, port):
        super().__init__(host, port)

    # 연결시 시작 flag를 보낸다.
    def makeSocket(self, timeout=1):
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            clientSocket.connect(self.address)
            clientSocket.send(b"\xFF\xFF\xFF\xFF")
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord 필요한 데이터 바이트 전송
    def makePickle(self, record) -> bytes:
        # len | logger | int          | len | message | log4cxx_time_t | len | threadName | len | fileName | len | methodName | len | lineNumber
        #   name       | int(levelno) |    msg        | int(created)   |  threadName      |  pathname      |  funcName        |  str(lineno)

        nameLen = len(record.name.encode())
        msgLen = len(record.msg.encode())
        threadNameLen = len(record.threadName.encode())
        pathnameLen = len(record.pathname.encode())
        funcNameLen = len(record.funcName.encode())
        linenoLen = len(str(record.lineno).encode())
        struct_fmt = "! I %ds i I %ds Q I %ds I %ds I %ds I %ds" % (nameLen, msgLen, threadNameLen, pathnameLen, funcNameLen, linenoLen)
        data = struct.pack(
            struct_fmt,
            nameLen,
            record.name.encode(),
            int(record.levelno) * 1000,
            msgLen,
            record.msg.encode(),
            round(record.created * 1000 * 1000),
            threadNameLen,
            record.threadName.encode(),
            pathnameLen,
            record.pathname.encode(),
            funcNameLen,
            record.funcName.encode(),
            linenoLen,
            str(record.lineno).encode(),
        )
        dataLen = len(data)
        self.debugWrite("record.bin", data)

        # 현재는 보내지 않음
        # 보낼려면 return dataLen + data
        dataLen = struct.pack("!L", dataLen)
        return data

    def debugWrite(self, fileName, data):
        f = open(fileName, "wb")
        f.write(data)
        f.close()


#####
# msgpackSocketHandler
#####
class msgpackSocketHandler(logging.handlers.SocketHandler):
    def __init__(self, host, port):
        super().__init__(host, port)

    # 연결시 시작 flag를 보낸다.
    def makeSocket(self, timeout=1):
        clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            clientSocket.connect(self.address)
            clientSocket.send(b"\xFF\xFF\xFF\xFF")
        except OSError:
            clientSocket.close()
            raise
        return clientSocket

    # LogRecord -> msgpack 바이트 전송
    def makePickle(self, record) -> bytes:
        data = msgpack.packb(record.__dict__)
        # 현재는 보내지 않음
        # 보낼려면 return dataLen + data
        dataLen = struct.pack("!L", len(data))
        return data

    def debugWrite(self, fileName, data):
        f = open(fileName, "wb")
        f.write(data)
        f.close()


#####
# jsonSocketHandler
#####
class jsonSocketHandler(logging.handlers.SocketHandler):
    def __init__(self, host, port):
        super().__init__(host, port)

    def makePickle(self, record) -> bytes:
        return super().makePickle(record)
