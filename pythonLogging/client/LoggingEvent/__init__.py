import logging.handlers

from .DefaultSocketHandler import *
from .ByteSocketHandler import *
from .MsgpackSocketHandler import *
from .JsonSocketHandler import *

#__all__ = ['DefaultSocketHandler', 'ByteSocketHandler',
#           'MsgpackSocketHandler', 'JsonSocketHandler']

LOGGING_EVENT_DEFAULT = 0
LOGGING_EVENT_BYTE = 1
LOGGING_EVENT_MSGPACK = 2
LOGGING_EVENT_JSON = 3

def createLoggingEventHandler(
    type = LOGGING_EVENT_DEFAULT, 
    host = "localhost", 
    port = logging.handlers.DEFAULT_TCP_LOGGING_PORT
) -> logging.handlers.SocketHandler:

    if type == LOGGING_EVENT_DEFAULT:
        return DefaultSocketHandler(host, port)
    elif type == LOGGING_EVENT_BYTE:
        return  ByteSocketHandler(host, port)
    elif type == LOGGING_EVENT_MSGPACK:
        return  MsgpackSocketHandler(host, port)
    elif type == LOGGING_EVENT_JSON:
        return  JsonSocketHandler(host, port)

    return None
