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

package da16200_tls_client_sample;

import java.net.InetAddress;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import org.bouncycastle.tls.TlsClient;
import org.bouncycastle.tls.TlsClientProtocol;

public class TLSEchoClient {
	private static int MAX_DATA_TRANSMISSION = 100;
	private static  int DEF_WAITTIME = 5 * 1000; // 5sec
	
    private static String TLS_SERVER_DEF_IP_ADDR = "192.168.0.2";
	private static int TLS_SERVER_DEF_PORT = 10197;

	private static String VERSION = "1.0";
	
	public static void displayInformation()
	{
		System.out.println("************************************************************");
		System.out.println("* TLS Client");
		System.out.println("* ver. " + VERSION);
		System.out.println("* Usage: tls_client.exe [TLS Server IP Address] [Port]");
		System.out.println("************************************************************");
		System.out.println("");
	}

	public static void main(String[] args) throws Exception {
		int count = 0;
		String defMsg = "Hello";
		byte[] recvBuffer = new byte[1024];
		
		SimpleTlsClient client = new SimpleTlsClient(null);
		TlsClientProtocol protocol = null;
		
		String peer_ip_str = TLS_SERVER_DEF_IP_ADDR;
		int peer_port = TLS_SERVER_DEF_PORT;
		
		if (args.length == 1) {
			peer_ip_str = args[0];
		} else if (args.length == 2 ) {
			peer_ip_str = args[0];
			peer_port = Integer.parseInt(args[1]);
		}
		
		displayInformation();
		
		System.out.println("Server IP Address: " + peer_ip_str);
		System.out.println("Server Port: " + peer_port);

		InetAddress peer_address = InetAddress.getByName(peer_ip_str);
		
		protocol = openTlsConnection(peer_address, peer_port, client);
		
		InputStream input = protocol.getInputStream();
		OutputStream output = protocol.getOutputStream();

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
		
		protocol.close();
	}
	
	static TlsClientProtocol openTlsConnection(InetAddress address, int port, TlsClient client) throws IOException
	{
		Socket socket = new Socket(address, port);
		TlsClientProtocol protocol = new TlsClientProtocol(socket.getInputStream(), socket.getOutputStream());
		protocol.connect(client);
		return protocol;
	}
}
