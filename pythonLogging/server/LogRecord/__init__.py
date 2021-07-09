from .DefaultLogRecordStreamHandler import *
from .BytesLogRecordStreamHandler import *
from .MsgpackLogRecordStreamHandler import *
from .JsonLogRecordStreamHandler import *
import logging
import pickle

#__all__ = ['DefaultLogRecordStreamHandler', 'BytesLogRecordStreamHandler',
#           'MsgpackLogRecordStreamHandler', 'JsonLogRecordStreamHandler']

def debugWrite(fileName : str, data : bytes):
    f = open(fileName, "wb")
    f.write(data)
    f.close()

def writeChunk(fileName : str, data : bytes):
    f = open(fileName, "wb")
    f.write(data)
    f.close()

def writeRecord(fileName : str, data : logging.LogRecord):
    f = open(fileName, "wb")
    pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)
    f.close()

# 폴더 생성
"""
directory = "../pkl"
try:
    if not os.path.exists(directory):
        os.makedirs(directory)
except OSError:
    print("Error: Creating directory. " + directory)
cnt = 0
"""

# pickle 처리
"""
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
"""

"""
def unPickle(self, data):
    return pickle.loads(data)
"""