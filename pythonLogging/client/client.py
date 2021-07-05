# client.py
import logging
import logging.handlers
import socketHandlers
import time


def createSocketHandler(str="bytes") -> logging.handlers.SocketHandler:
    socketHandler = None
    if str == "bytes":
        socketHandler = socketHandlers.bytesSocketHandler("localhost", 9988)
    elif str == "msgpack":
        socketHandler = socketHandlers.msgpackSocketHandler("localhost", 9988)
    elif str == "json":
        socketHandler = socketHandlers.jsonSocketHandler("localhost", 9988)
    else:
        socketHandler = logging.handlers.SocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)

    return socketHandler


def main():
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)

    # don't bother with a formatter, since a socket handler sends the event as
    # an unformatted pickle
    rootLogger.addHandler(createSocketHandler("msgpack"))

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
