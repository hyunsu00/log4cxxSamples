# server.py
import pickle
import logging
import logging.handlers
import socketserver
import struct
import os
import msgpackLogRecord


class LogRecordStreamHandler(socketserver.StreamRequestHandler):
    """Handler for a streaming logging request.

    This basically logs the record using whatever logging policy is
    configured locally.
    """

    def handle(self):
        """
        Handle multiple requests - each expected to be a 4-byte length,
        followed by the LogRecord in pickle format. Logs the record
        according to whatever policy is configured locally.
        """

        # 폴더 생성
        directory = "../pkl"
        try:
            if not os.path.exists(directory):
                os.makedirs(directory)
        except OSError:
            print('Error: Creating directory. ' + directory)
        cnt = 0

        # pickle 처리
        '''
        while True:
            chunk = self.connection.recv(4)
            if len(chunk) < 4:
                break
            slen = struct.unpack('>L', chunk)[0]
            chunk = self.connection.recv(slen)
            while len(chunk) < slen:
                chunk = chunk + self.connection.recv(slen - len(chunk))
            obj = self.unPickle(chunk)
            record = logging.makeLogRecord(obj)
            self.handleLogRecord(record)
            # 패킷 pickle 데이터 파일로 저장
            self.writeChunk(directory + '/chunk{0}.pkl'.format(cnt), chunk)
            self.writeRecord(directory + '/record{0}.pkl'.format(cnt), record)
            cnt += 1
        '''

        # msgpack 처리
        # '''
        import msgpack
        unpacker = msgpack.Unpacker(use_list=False, raw=False)
        while True:
            data = self.connection.recv(1024)
            if not data:
                break
            unpacker.feed(data)
            for unpacked in unpacker:
                record = msgpackLogRecord.createLogRecord(unpacked)
                self.handleLogRecord(record)
        # '''

    def unPickle(self, data):
        return pickle.loads(data)

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

    def writeChunk(self, fileName, data):
        f = open(fileName, 'wb')
        f.write(data)
        f.close()

    def writeRecord(self, fileName, data):
        f = open(fileName, 'wb')
        pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)
        f.close()


class LogRecordSocketReceiver(socketserver.ThreadingTCPServer):
    """
    Simple TCP socket-based logging receiver suitable for testing.
    """

    allow_reuse_address = 1

    def __init__(self, host='localhost',
                 port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                 handler=LogRecordStreamHandler):
        socketserver.ThreadingTCPServer.__init__(self, (host, port), handler)
        self.abort = 0
        self.timeout = 1
        self.logname = None

    def serve_until_stopped(self):
        import select
        abort = 0
        while not abort:
            rd, wr, ex = select.select([self.socket.fileno()],
                                       [], [],
                                       self.timeout)
            if rd:
                self.handle_request()
            abort = self.abort


def main():
    logging.basicConfig(
        format='%(relativeCreated)5d %(name)-15s %(levelname)-8s %(message)s')
    tcpserver = LogRecordSocketReceiver()
    print('About to start TCP server...')
    tcpserver.serve_until_stopped()


if __name__ == '__main__':
    main()
