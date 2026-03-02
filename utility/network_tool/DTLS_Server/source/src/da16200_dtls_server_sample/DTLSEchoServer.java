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

package da16200_dtls_server_sample;

import java.io.IOException;
import java.io.InputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.security.SecureRandom;
import java.util.Enumeration;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.bouncycastle.tls.AlertDescription;
import org.bouncycastle.tls.DTLSRequest;
import org.bouncycastle.tls.DTLSServerProtocol;
import org.bouncycastle.tls.DTLSTransport;
import org.bouncycastle.tls.DTLSVerifier;
import org.bouncycastle.tls.DatagramSender;
import org.bouncycastle.tls.DatagramTransport;
import org.bouncycastle.tls.TlsFatalAlert;
import org.bouncycastle.tls.UDPTransport;
import org.bouncycastle.tls.crypto.impl.bc.BcTlsCrypto;

public class DTLSEchoServer {
	private static int MAX_DATA_TRANSMISSION = 100;
	private static  int DEF_WAITTIME = 5 * 1000; // 5 sec
	
	private static int DTLS_SERVER_DEF_PORT = 10199;

	private static String VERSION = "1.0";
	
	public static void displayInformation()
	{
		System.out.println("************************************************************");
		System.out.println("* DTLS Server");
		System.out.println("* ver. " + VERSION);
		System.out.println("* Usage: dtls_server.exe [IP address] [Port]");
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
        final int mtu = 1500;
        
		InetAddress local_addr = InetAddress.getByName("0.0.0.0");
		int port = 0;

		int count = 0;
		String defMsg = "Hello";
		byte[] recvBuffer = new byte[1024];

        DTLSVerifier verifier = new DTLSVerifier(new BcTlsCrypto(new SecureRandom()));
        DTLSRequest request = null;
        
		//check parameters
		if (args.length == 0) {
			local_addr = InetAddress.getLocalHost();
			port = DTLS_SERVER_DEF_PORT;
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


        byte[] data = new byte[mtu];
        final DatagramPacket packet = new DatagramPacket(data, mtu);
        final DatagramSocket socket = new DatagramSocket(port, local_addr);
        
        displayInformation();
        
        System.out.println("Bind on udp/" + local_addr.getHostAddress() + ":" + port);

        /*
         * Process incoming packets, replying with HelloVerifyRequest, until one is verified.
         */
        do
        {
            socket.receive(packet);

            request = verifier.verifyRequest(packet.getAddress().getAddress(), data, 0, packet.getLength(), new DatagramSender()
            {
                public int getSendLimit() throws IOException
                {
                    return mtu - 28;
                }

                public void send(byte[] buf, int off, int len) throws IOException
                {
                    if (len > getSendLimit())
                    {
                        throw new TlsFatalAlert(AlertDescription.internal_error);
                    }

                    socket.send(new DatagramPacket(buf, off, len, packet.getAddress(), packet.getPort()));
                }
            });
        } while (null == request);

        /*
         * Proceed to a handshake, passing verified 'request' (ClientHello) to DTLSServerProtocol.accept.
         */
        System.out.println("Accepting connection from " + packet.getAddress().getHostAddress() + ":" + packet.getPort());
        socket.connect(packet.getAddress(), packet.getPort());

        DatagramTransport transport = new UDPTransport(socket, mtu);

        SimpleDtlsServer server = new SimpleDtlsServer();
        DTLSServerProtocol serverProtocol = new DTLSServerProtocol();
        
        DTLSTransport dtlsServer = serverProtocol.accept(server, transport, request);

        while (!socket.isClosed() && count < MAX_DATA_TRANSMISSION)
        {
            try
            {
    			//Send data
    			byte[] sentMsg = defMsg.getBytes();
    			dtlsServer.send(sentMsg, 0, sentMsg.length);
    			
    			System.out.println("Sent: " + new String(sentMsg) + "(" + sentMsg.length + ")");
    			
    			//Read data
    			int recvLen = dtlsServer.receive(recvBuffer, 0, recvBuffer.length, 50000);
    			if (recvLen > 0)
    			{
    				System.out.println("Recv: " + new String(recvBuffer).trim() + "(" + recvLen + ")");
    			}
    			
    			count++;
    			
    			Thread.sleep(DEF_WAITTIME);
            }
            catch (SocketTimeoutException ste)
            {
            }
        }

        dtlsServer.close();
	}
}
