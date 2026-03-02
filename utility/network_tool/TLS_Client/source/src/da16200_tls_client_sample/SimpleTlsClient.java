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

import java.io.IOException;
import java.io.PrintStream;
import java.security.SecureRandom;
import java.util.Hashtable;
import java.util.Vector;

import org.bouncycastle.tls.AlertDescription;
import org.bouncycastle.tls.AlertLevel;
import org.bouncycastle.tls.CertificateRequest;
import org.bouncycastle.tls.ChannelBinding;
import org.bouncycastle.tls.DefaultTlsClient;
import org.bouncycastle.tls.ProtocolName;
import org.bouncycastle.tls.ProtocolVersion;
import org.bouncycastle.tls.TlsAuthentication;
import org.bouncycastle.tls.TlsCredentials;
import org.bouncycastle.tls.TlsExtensionsUtils;
import org.bouncycastle.tls.TlsServerCertificate;
import org.bouncycastle.tls.TlsSession;
import org.bouncycastle.tls.crypto.impl.bc.BcTlsCrypto;
import org.bouncycastle.util.Arrays;
import org.bouncycastle.util.encoders.Hex;

public class SimpleTlsClient extends DefaultTlsClient {
	TlsSession session;
	
	SimpleTlsClient(TlsSession session)
	{
		super(new BcTlsCrypto(new SecureRandom()));
		
		this.session = session;
	}
	

    protected Vector getProtocolNames()
    {
        Vector protocolNames = new Vector();
        protocolNames.addElement(ProtocolName.HTTP_1_1);
        protocolNames.addElement(ProtocolName.HTTP_2_TLS);
        return protocolNames;
    }

    public TlsSession getSessionToResume()
    {
        return this.session;
    }

    public void notifyAlertRaised(short alertLevel, short alertDescription, String message, Throwable cause)
    {
        PrintStream out = (alertLevel == AlertLevel.fatal) ? System.err : System.out;
        out.println("TLS client raised alert: " + AlertLevel.getText(alertLevel)
            + ", " + AlertDescription.getText(alertDescription));
        if (message != null)
        {
            out.println("> " + message);
        }
        if (cause != null)
        {
            cause.printStackTrace(out);
        }
    }
    public Hashtable getClientExtensions() throws IOException
    {
        Hashtable clientExtensions = TlsExtensionsUtils.ensureExtensionsInitialised(super.getClientExtensions());
        return clientExtensions;
    }

    public void notifyServerVersion(ProtocolVersion serverVersion) throws IOException
    {
        super.notifyServerVersion(serverVersion);
    }

    public TlsAuthentication getAuthentication() throws IOException
    {
        return new TlsAuthentication()
        {
            public void notifyServerCertificate(TlsServerCertificate serverCertificate)
                throws IOException
            {
            	return ;
            }

            public TlsCredentials getClientCredentials(CertificateRequest certificateRequest)
                throws IOException
            {
            	return null;
            }
        };
    }
    
    public void notifyHandshakeComplete() throws IOException
    {
        super.notifyHandshakeComplete();

        ProtocolName protocolName = context.getSecurityParametersConnection().getApplicationProtocol();
        if (protocolName != null)
        {
            System.out.println("Client ALPN: " + protocolName.getUtf8Decoding());
        }

        TlsSession newSession = context.getResumableSession();
        if (newSession != null)
        {
            byte[] newSessionID = newSession.getSessionID();
            String hex = Hex.toHexString(newSessionID);

            if (this.session != null && Arrays.areEqual(this.session.getSessionID(), newSessionID))
            {
                System.out.println("Client resumed session: " + hex);
            }
            else
            {
                System.out.println("Client established session: " + hex);

                byte[] tlsServerEndPoint = context.exportChannelBinding(ChannelBinding.tls_server_end_point);
                System.out.println("Client 'tls-server-end-point': " + hex(tlsServerEndPoint));
            }

            byte[] tlsUnique = context.exportChannelBinding(ChannelBinding.tls_unique);
            System.out.println("Client 'tls-unique': " + hex(tlsUnique));

            this.session = newSession;
        }
    }

    protected String hex(byte[] data)
    {
        return data == null ? "(null)" : Hex.toHexString(data);
    }
}
