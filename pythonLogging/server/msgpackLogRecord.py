# msgpackLogRecord.py
import logging


class magpackLogRecord(logging.LogRecord):
    def __init__(self, unpacked):
        super().__init__(
            unpacked["name"], unpacked["levelno"], unpacked["pathname"], unpacked["lineno"], unpacked["msg"], unpacked["args"], unpacked["exc_info"], unpacked["funcName"], unpacked["stack_info"]
        )
        # self.name = unpacked['name'] # str
        # self.levelno = unpacked['levelno'] # str
        # self.pathname = unpacked['pathname'] # str
        # self.lineno = unpacked['lineno'] # int
        # self.msg = unpacked['msg'] # str
        # self.args = unpacked['args'] # _ArgsType
        # self.exc_info = unpacked['exc_info'] # float
        # self.funcName = unpacked['funcName'] # str
        # self.stack_info = unpacked['stack_info'] # Optional[str]

        self.process = unpacked["process"]  # Optional[int]
        self.processName = unpacked["processName"]  # Optional[str]
        self.thread = unpacked["thread"]  # Optional[int]
        self.threadName = unpacked["threadName"]  # Optional[str]
        self.exc_text = unpacked["exc_text"]  # Optional[_SysExcInfoType]

        self.levelname = unpacked["levelname"]  # str
        self.filename = unpacked["filename"]  # Optional[str]
        self.module = unpacked["module"]  # str

        self.created = unpacked["created"]  # str
        self.msecs = unpacked["msecs"]  # float
        self.relativeCreated = unpacked["relativeCreated"]  # float


def createLogRecord(unpacked) -> logging.LogRecord:
    return magpackLogRecord(unpacked)
