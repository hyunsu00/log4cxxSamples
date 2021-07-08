from .DefaultLogRecord import *
from .BytesLogRecord import *
from .MsgpackLogRecord import *
from .JsonLogRecord import *

#__all__ = ['DefaultLogRecord', 'BytesLogRecord',
#           'MsgpackLogRecord', 'JsonLogRecord']

def debugWrite(fileName : str, data : bytes):
    f = open(fileName, "wb")
    f.write(data)
    f.close()