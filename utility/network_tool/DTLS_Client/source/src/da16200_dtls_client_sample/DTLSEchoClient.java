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

package da16200_dtls_client_sample;

import java.io.IOException;
import java.net.DatagramSocket;
import java.net.InetAddress;

import org.bouncycastle.tls.DTLSClientProtocol;
import org.bouncycastle.tls.DTLSTransport;
import org.bouncycastle.tls.DatagramTransport;
import org.bouncycastle.tls.TlsClient;
import org.bouncycastle.tls.UDPTransport;


public class DTLSEchoClient {
	private static int MAX_DATA_TRANSMISSION = 100;
	private static  int DEF_WAITTIME = 5 * 1000; // 5sec

	private static String DTLS_SERVER_DEF_IP_ADDR = "192.168.0.2";
	private static int DTLS_SERVER_DEF_PORT = 10199;

	private static String VERSION = "1.0";

	public static void displayInformation()
	{
		System.out.println("************************************************************");
		System.out.println("* DTLS Client");
		System.out.println("* ver. " + VERSION);
		System.out.println("* Usage: dtls_client.exe [DTLS Server IP Address] [Port]");
		System.out.println("************************************************************");
		System.out.println("");
	}

	public static void main(String[] args) throws Exception {
		int count = 0;
		String defMsg = "Hello";
		byte[] recvBuffer = new byte[1024];

		SimpleDtlsClient client = new SimpleDtlsClient(null);
		DTLSTransport dtls = null;

		String peer_ip_str = DTLS_SERVER_DEF_IP_ADDR;
		int peer_port = DTLS_SERVER_DEF_PORT;

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

		dtls = openDTLSConnection(peer_address, peer_port, client);

		// Send and hopefully receive a packet back
		while (count < MAX_DATA_TRANSMISSION) {
			//Send data
			byte[] sentMsg = defMsg.getBytes();
			dtls.send(sentMsg, 0, sentMsg.length);

			System.out.println("Sent: " + new String(sentMsg) + "(" + sentMsg.length + ")");

			//Read data
			int recvLen = dtls.receive(recvBuffer, 0, recvBuffer.length, 50000);
			if (recvLen > 0)
			{
				System.out.println("Recv: " + new String(recvBuffer).trim() + "(" + recvLen + ")");
			}

			count++;

			Thread.sleep(DEF_WAITTIME);
		}

		dtls.close();
	}

	private static DTLSTransport openDTLSConnection(InetAddress address, int port, TlsClient client) throws IOException
	{
		DatagramSocket socket = new DatagramSocket();
		socket.connect(address, port);

		int mtu = 1500;
		DatagramTransport transport = null;

		transport = new UDPTransport(socket, mtu);

		DTLSClientProtocol protocol = new DTLSClientProtocol();

		return protocol.connect(client, transport);
	}
}
