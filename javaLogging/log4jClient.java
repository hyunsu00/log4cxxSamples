// log4jClient.java
import org.apache.log4j.Logger;
import org.apache.log4j.PatternLayout;
import org.apache.log4j.ConsoleAppender;
import org.apache.log4j.Level;
import org.apache.log4j.net.SocketAppender;

public class log4jClient  {
	//static public final int PORT = 9988; 
	static public final int PORT = 9988;
	private static final Logger clientLogger = Logger.getLogger(log4jClient.class);
	public static void main(String args[]) {
		// BasicConfigurator.configure();
		Logger.getRootLogger().addAppender(
			new ConsoleAppender(
				new PatternLayout("%5p %F\\:%L [%d] - %m%n")
			)
		);
		Logger.getRootLogger().addAppender(
			new SocketAppender("localhost", PORT)
		);

		Logger.getRootLogger().setLevel(Level.INFO);
		try {
			Logger rootLogger = Logger.getRootLogger();
			while (true) {
				rootLogger.trace("[rootLogger(wchar_t)] : TRACE 출력 שלום (שלום)...");
				rootLogger.debug("[rootLogger(wchar_t)] : DEBUG 출력 Сәлем (Sälem)...");
				rootLogger.info("[rootLogger(wchar_t)] : INFO 출력 مرحبًا (mrhbana)...");
				rootLogger.warn("[rootLogger(wchar_t)] : WARN 출력 안녕하세요 (annyeonghaseyo)...");
				rootLogger.error("[rootLogger(wchar_t)] : ERROR 출력 こんにちは (Kon'nichiwa)...");
				rootLogger.fatal("[rootLogger(wchar_t)] : FATAL 출력 你好 (Nǐ hǎo)...");

				rootLogger.trace("[" + rootLogger.getName() + "(LogString)] : TRACE 출력 שלום (שלום)...");	
				rootLogger.debug("[" + rootLogger.getName() + "(wchar_t)] : DEBUG 출력 Сәлем (Sälem)...");
				rootLogger.info("[" + rootLogger.getName() + "(wchar_t)] : INFO 출력 مرحبًا (mrhbana)...");
				rootLogger.warn("[" + rootLogger.getName() + "(wchar_t)] : WARN 출력 안녕하세요 (annyeonghaseyo)...");
				rootLogger.error("[" + rootLogger.getName() + "(wchar_t)] : ERROR 출력 こんにちは (Kon'nichiwa)...");
				rootLogger.fatal("[" + rootLogger.getName() + "(wchar_t)] : FATAL 출력 你好 (Nǐ hǎo)...");
				
				Thread.sleep(1000);
				//Thread.yield();
			}
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
}