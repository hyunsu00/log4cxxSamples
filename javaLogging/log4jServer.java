// log4jServer.java
import org.apache.log4j.Logger; // Logger
import org.apache.log4j.ConsoleAppender; // ConsoleAppender
import org.apache.log4j.PatternLayout; // PatternLayout
import org.apache.log4j.LogManager; // LogManager
import org.apache.log4j.net.SocketNode; // ScketNode

import java.net.ServerSocket; // ServerSocket
import java.net.Socket; // Socket

public class log4jServer {
	static public final int PORT = 9988;  
	static Logger serverLogger = Logger.getLogger(log4jServer.class);
	public static void main(String args[]) {
		// BasicConfigurator.configure();
    	Logger.getRootLogger().addAppender(
			new ConsoleAppender(
				new PatternLayout("%5p %F\\:%L [%d] - %m%n")
			)
		);

		try {
			serverLogger.info("Listening on port " + PORT);
			ServerSocket serverSocket = new ServerSocket(PORT);
			while (true) {
				serverLogger.info("Waiting to accept a new client.");
				Socket clientSocket = serverSocket.accept();
				serverLogger.info("Connected to client at " + clientSocket.getInetAddress());
        		serverLogger.info("Starting new socket node.");
				SocketNode clientSocketNode = new SocketNode(clientSocket, LogManager.getLoggerRepository());
				Thread clientSocketThread = new Thread(clientSocketNode);
				clientSocketThread.start(); 
				clientSocketThread.join();
			}	
		} catch(Exception e) {
			e.printStackTrace();
		}
	}
}
