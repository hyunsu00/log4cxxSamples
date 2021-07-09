import socketserver
import logging
import struct

BYTE_START_FLAG = b"\xAC\xED\x01\x05"

class BytesLogRecordStreamHandler(socketserver.StreamRequestHandler):
    """description of class"""
    def handle(self):
        # bytes 처리
        data = self.connection.recv(4)
        if data != BYTE_START_FLAG:
            return

        while True:
            nameLen, = struct.unpack("! I", self.connection.recv(4))
            name, = struct.unpack("! %ds" % nameLen, self.connection.recv(nameLen)); name = name.decode('utf-8')
            levelno, = struct.unpack("! I", self.connection.recv(4)); levelno = int(levelno / 1000)
            msgLen, = struct.unpack("! I", self.connection.recv(4))
            msg, = struct.unpack("! %ds" % msgLen, self.connection.recv(msgLen)); msg = msg.decode('utf-8')
            created, = struct.unpack("! Q", self.connection.recv(8)); created = created / 1000 / 1000
            threadNameLen, = struct.unpack("! I", self.connection.recv(4))
            threadName, = struct.unpack("! %ds" % threadNameLen, self.connection.recv(threadNameLen)); threadName = threadName.decode('utf-8')
            pathnameLen, = struct.unpack("! I", self.connection.recv(4))
            pathname, = struct.unpack("! %ds" % pathnameLen, self.connection.recv(pathnameLen)); pathname = pathname.decode('utf-8')
            funcNameLen, = struct.unpack("! I", self.connection.recv(4))
            funcName, = struct.unpack("! %ds" % funcNameLen, self.connection.recv(funcNameLen)); funcName = funcName.decode('utf-8')
            linenoLen, = struct.unpack("! I", self.connection.recv(4))
            lineno, = struct.unpack("! %ds" % linenoLen, self.connection.recv(linenoLen)); lineno = int(lineno)
            unpacked = dict(name = name, levelno = levelno, msg = msg, created = created, threadName = threadName, pathname = pathname, funcName = funcName, lineno = lineno)
            record = self.createLogRecord(unpacked)
            self.handleLogRecord(record)

    def createLogRecord(self, unpacked : dict) -> logging.LogRecord:
        logRecord = logging.LogRecord(
            name = unpacked["name"], level = unpacked["levelno"], pathname = unpacked["pathname"], 
            lineno = unpacked["lineno"], msg = unpacked["msg"], args = None, 
            exc_info = None, func = unpacked["funcName"], sinfo = None
        )
        # logRecord.name = unpacked['name'] # str
        # logRecord.levelno = unpacked['levelno'] # int
        # logRecord.pathname = unpacked['pathname'] # str
        # logRecord.lineno = unpacked['lineno'] # int
        # logRecord.msg = unpacked['msg'] # str
        # logRecord.args = unpacked['args'] # _ArgsType
        # logRecord.exc_info = unpacked['exc_info'] # float
        # logRecord.funcName = unpacked['funcName'] # str
        # logRecord.stack_info = unpacked['stack_info'] # Optional[str]

        # logRecord.process = unpacked["process"]  # Optional[int]
        # logRecord.processName = unpacked["processName"]  # Optional[str]
        # logRecord.thread = unpacked["thread"]  # Optional[int]
        logRecord.threadName = unpacked["threadName"]  # Optional[str]
        # logRecord.exc_text = unpacked["exc_text"]  # Optional[_SysExcInfoType]

        # logRecord.levelname = unpacked["levelname"]  # str
        # logRecord.filename = unpacked["filename"]  # Optional[str]
        # logRecord.module = unpacked["module"]  # str

        logRecord.created = unpacked["created"]  # float
        # logRecord.msecs = unpacked["msecs"]  # float
        # logRecord.relativeCreated = unpacked["relativeCreated"]  # float

        return logRecord

    def handleLogRecord(self, record):
        # if a name is specified, we use the named logger rather than the one
        # implied by the record.
        if self.server.logname is not None:
            name = self.server.logname
        else:
            name = record.name
        logger = logging.getLogger(name)
        # N.B. EVERY record gets logged. This is because Logger.handle
        # is normally called AFTER logger-level filtering. If you want
        # to do filtering, do it at the client end to save wasting
        # cycles and network bandwidth!
        logger.handle(record)
