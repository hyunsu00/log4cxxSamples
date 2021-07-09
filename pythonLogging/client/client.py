# client.py
import logging
import time
from LoggingEvent import *

def main():
    logging.basicConfig(format="%(relativeCreated)5d %(name)-15s %(levelname)-8s %(message)s")
    rootLogger = logging.getLogger("")
    rootLogger.setLevel(logging.DEBUG)

    # don't bother with a formatter, since a socket handler sends the event as
    # an unformatted pickle
    rootLogger.addHandler(createLoggingEventHandler(LOGGING_EVENT_DEFAULT, "localhost", 9988))

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
