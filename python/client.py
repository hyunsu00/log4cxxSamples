# client.py
import logging
import logging.handlers
import socketHandlers


def createSocketHandler(str="bytes") -> logging.handlers.SocketHandler:
    socketHandler = None
    if str == "bytes":
        socketHandler = socketHandlers.bytesSocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)
    elif str == "msgpack":
        socketHandler = socketHandlers.msgpackSocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)
    elif str == "json":
        socketHandler = socketHandlers.jsonSocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)
    else:
        socketHandler = logging.handlers.SocketHandler("localhost", logging.handlers.DEFAULT_TCP_LOGGING_PORT)

    return socketHandler


def main():
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)

    # don't bother with a formatter, since a socket handler sends the event as
    # an unformatted pickle
    rootLogger.addHandler(createSocketHandler("bytes"))

    # Now, we can log to the root logger, or any other logger. First the root...
    logging.info("Jackdaws love my big sphinx of quartz.")

    # Now, define a couple of other loggers which might represent areas in your
    # application:
    logger1 = logging.getLogger("myapp.area1")
    logger2 = logging.getLogger("myapp.area2")

    logger1.debug("Quick zephyrs blow, vexing daft Jim.")
    logger1.info("How quickly daft jumping zebras vex.")
    logger2.warning("Jail zesty vixen who grabbed pay from quack.")
    logger2.error("The five boxing wizards jump quickly.")


if __name__ == "__main__":
    main()
