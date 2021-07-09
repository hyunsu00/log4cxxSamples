import socketserver
import logging
import msgpack

MSGPACK_START_FLAG = b"\xAC\xED\x02\x05"
MSGPACK_DEFAULT_BUFFER_LEN = 1024

class MsgpackLogRecordStreamHandler(socketserver.StreamRequestHandler):
    """description of class"""
    def handle(self):
        # msgpack 처리
        data = self.connection.recv(4)
        if data != MSGPACK_START_FLAG:
            return

        unpacker = msgpack.Unpacker(use_list=False, raw=False)
        while True:
            data = self.connection.recv(MSGPACK_DEFAULT_BUFFER_LEN)
            if not data:
                break
            unpacker.feed(data)
            for unpacked in unpacker:
                record = self.createLogRecord(unpacked)
                self.handleLogRecord(record)

    def createLogRecord(self, unpacked : dict) -> logging.LogRecord:
        logRecord = logging.LogRecord(
            unpacked["name"], unpacked["levelno"], unpacked["pathname"], 
            unpacked["lineno"], unpacked["msg"], unpacked["args"], 
            unpacked["exc_info"], unpacked["funcName"], unpacked["stack_info"]
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

        logRecord.process = unpacked["process"]  # Optional[int]
        logRecord.processName = unpacked["processName"]  # Optional[str]
        logRecord.thread = unpacked["thread"]  # Optional[int]
        logRecord.threadName = unpacked["threadName"]  # Optional[str]
        logRecord.exc_text = unpacked["exc_text"]  # Optional[_SysExcInfoType]

        logRecord.levelname = unpacked["levelname"]  # str
        logRecord.filename = unpacked["filename"]  # Optional[str]
        logRecord.module = unpacked["module"]  # str

        logRecord.created = unpacked["created"]  # float
        logRecord.msecs = unpacked["msecs"]  # float
        logRecord.relativeCreated = unpacked["relativeCreated"]  # float

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
