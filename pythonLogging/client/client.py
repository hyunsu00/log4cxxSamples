# client.py
import logging
import logging.handlers
import time
from LoggingEvent import DefaultSocketHandler, ByteSocketHandler, MsgpackSocketHandler, JsonSocketHandler

def createSocketHandler(str="Default") -> logging.handlers.SocketHandler:
    socketHandler = None
    if str == "Default":
        socketHandler = DefaultSocketHandler.DefaultSocketHandler("localhost", 9988)
    elif str == "Bytes":
        socketHandler = ByteSocketHandler.ByteSocketHandler("localhost", 9988)
    elif str == "Msgpack":
        socketHandler = MsgpackSocketHandler.MsgpackSocketHandler("localhost", 9988)
    elif str == "Json":
        socketHandler = JsonSocketHandler.JsonSocketHandler("localhost", 9988)
    else:
        socketHandler = logging.handlers.SocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)

    return socketHandler


def main():
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)

    # don't bother with a formatter, since a socket handler sends the event as
    # an unformatted pickle
    rootLogger.addHandler(createSocketHandler("Default"))

    # Now, we can log to the root logger, or any other logger. First the root...
    logging.info("INFO 출력 مرحبًا (mrhbana)...")
    logging.critical("FATAL 출력 你好 (Nǐ hǎo)...")

    # Now, define a couple of other loggers which might represent areas in your
    # application:
    logger1 = logging.getLogger("myapp.area1")
    logger2 = logging.getLogger("myapp.area2")

    while True:
        logger1.debug("DEBUG 출력 안녕하세요 (annyeonghaseyo)...")
        logger1.info("INFO 출력 Сәлем (Sälem)...")
        logger2.warning("WARN 출력 你好 (Nǐ hǎo)...")
        logger2.error("ERROR 출력 こんにちは (Kon'nichiwa)...")
        time.sleep(1)


if __name__ == "__main__":
    main()
