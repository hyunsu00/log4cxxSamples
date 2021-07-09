import socketserver
import logging

JSON_START_FLAG = b"\xAC\xED\x03\x05"

class JsonLogRecordStreamHandler(socketserver.StreamRequestHandler):
    """description of class"""
    def handle(self):
        # json 처리
        data = self.connection.recv(4)
        if data != JSON_START_FLAG:
            return

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
