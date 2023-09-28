/**
 ****************************************************************************************
 * Copyright (c) 2000-2019 The Legion of the Bouncy Castle Inc. (http://www.bouncycastle.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2019 Modified by Dialog Semiconductor
 ****************************************************************************************
 */

package da16200_tls_server_sample;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.bouncycastle.tls.TlsServerProtocol;

public class TLSEchoServer {
	private static int TLS_SERVER_DEF_PORT = 10196;
	
	private static String VERSION = "1.0";
	
	public static void displayInformation()
	{
		System.out.println("************************************************************");
		System.out.println("* TLS Server");
		System.out.println("* ver. " + VERSION);
		System.out.println("* Usage: tls_server.exe [IP Address] [Port]");
		System.out.println("************************************************************");
		System.out.println("");
	}
	
	public static boolean isLoalIPAddress(String addr)
	{
		String IPADDRESS_PATTERN = 
				"(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)";

		Pattern pattern = Pattern.compile(IPADDRESS_PATTERN);
		Matcher matcher = pattern.matcher(addr);
		if (matcher.find()) {
			try {
				Enumeration Interfaces = NetworkInterface.getNetworkInterfaces();
				while(Interfaces.hasMoreElements())
				{
					NetworkInterface Interface = (NetworkInterface)Interfaces.nextElement();
					Enumeration Addresses = Interface.getInetAddresses();
					while(Addresses.hasMoreElements()) {
					    InetAddress Address = (InetAddress)Addresses.nextElement();
					    //System.out.println(Address.getHostAddress());
					    
					    if (addr.equals(Address.getHostAddress())) {
					    	return true;
					    }
					}
				}
			} catch (SocketException e) {
				e.printStackTrace();
			}
		}
		
		return false;
	}
	
	public static boolean isLocalPort(String port)
	{
		String PORT_PATTERN = "^[0-9]*$";
		Pattern pattern = Pattern.compile(PORT_PATTERN);
		Matcher matcher = pattern.matcher(port);
		if (matcher.find()) {
			return true;
		}
		
		return false;
	}
	
	public static void main(String[] args) throws Exception {
		InetAddress local_addr = InetAddress.getByName("0.0.0.0");
		int port = 0;
		
		//check parameters
		if (args.length == 0) {
			local_addr = InetAddress.getLocalHost();
			port = TLS_SERVER_DEF_PORT;
		} else if (args.length == 1) {
			local_addr = InetAddress.getLocalHost();
			
			if (isLocalPort(args[0])) {
				port = Integer.parseInt(args[0]);
			}
		} else if (args.length == 2) {
			if (isLoalIPAddress(args[0])) {
				local_addr = InetAddress.getByName(args[0]);
			}
			
			if (isLocalPort(args[1])) {
				port = Integer.parseInt(args[1]);	
			}
		}
		
		if (local_addr.getHostAddress().equals("0.0.0.0")) {
			System.out.println("Failed to get IP address");
			return ;
		}

		if (port == 0) {
			System.out.println("Failed to get port");
			return ;
		}
		
		displayInformation();
		
        System.out.println("Bind on tcp/" + local_addr.getHostAddress() + ":" + port);
		
		ServerSocket serverSocket = new ServerSocket(port, 1, local_addr);
		try {
			while (true) {
				Socket socket = serverSocket.accept();
				System.out.println("Accepted " + socket);
				ServerThread serverThread = new ServerThread(socket);
				serverThread.start();
			}
		} finally {
			serverSocket.close();
		}
	}
	
	static class ServerThread extends Thread {
		private final Socket socket;
		private final int MAX_DATA_TRANSMISSION = 100;
		private final int DEF_WAITTIME = 5 * 1000; // 5sec
		
		ServerThread(Socket socket) {
			this.socket = socket;
		}
		
		public void run() {
			try {
				String defMsg = "Hello";
				byte[] recvBuffer = new byte[1024];
				
				int count = 0;
				
				SimpleTlsServer server = new SimpleTlsServer();
				TlsServerProtocol serverProtocol = new TlsServerProtocol(socket.getInputStream(), socket.getOutputStream());
				serverProtocol.accept(server);
				
				InputStream input = serverProtocol.getInputStream();
				OutputStream output = serverProtocol.getOutputStream();

				while (count < MAX_DATA_TRANSMISSION) {
					//Send data
					String sentMsg = defMsg;
					output.write(sentMsg.getBytes());
					System.out.println("Sent: " + sentMsg + "(" + sentMsg.length() + ")");
					
					//Read data
					input.read(recvBuffer);
					String recvMsg = new String(recvBuffer);
					System.out.println("Recv: " + recvMsg.trim() + "(" + recvMsg.trim().length() + ")");
					
					count++;
					
					Thread.sleep(DEF_WAITTIME);
				}				
				
				serverProtocol.close();
			} catch (Exception e) {
				throw new RuntimeException(e);
			} finally {
				try {
					socket.close();
				} catch (IOException e) {
					
				} finally {
					
				}
			}
		}
	}
}