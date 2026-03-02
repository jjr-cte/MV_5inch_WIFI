package da16200_coap_server_sample;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.security.GeneralSecurityException;
import java.security.cert.Certificate;

import org.apache.log4j.PropertyConfigurator;
import org.eclipse.californium.core.CoapServer;
import org.eclipse.californium.core.network.CoapEndpoint;
import org.eclipse.californium.core.network.config.NetworkConfig;
import org.eclipse.californium.elements.util.SslContextUtil;
//import org.eclipse.californium.proxy.resources.ForwardingResource;
//import org.eclipse.californium.proxy.resources.ProxyCoapClientResource;
import org.eclipse.californium.scandium.DTLSConnector;
import org.eclipse.californium.scandium.config.DtlsConnectorConfig;
import org.eclipse.californium.scandium.dtls.CertificateType;
import org.eclipse.californium.scandium.dtls.pskstore.InMemoryPskStore;

public class CoapServerSample {
	private static String VERSION = "1.0";
	
	public static final String LOG_PROPERTIES = "log4j.properties";
	
	private static final int DEFAULT_PORT = 5684; 
	
	private static final char[] TRUST_STORE_PASSWORD = "rootPass".toCharArray();
	private static final String TRUST_STORE_LOCATION = "trustStore.jks";
	
	private static final char[] KEY_STORE_PASSWORD = "endPass".toCharArray();
	private static final String KEY_STORE_LOCATION = "keyStore.jks";
	
	private static final String SAMPLE_RES_NAME = "res";
	private static final String SAMPLE_OBS_RES_NAME = "obs_res";
	private static final String SAMPLE_PROXY_RES_NAME = "proxy_res";
	
	private CoapServer coapServer;
	private DTLSConnector dtlsConnector;
	
	private SampleResource sampleRes;
	private SampleObserveResource sampleObsRes;
	//private ForwardingResource sampleProxyRes;
	
	public CoapServerSample() {
		PropertyConfigurator.configure(LOG_PROPERTIES);
		
		coapServer = new CoapServer();
		
		sampleRes = new SampleResource(SAMPLE_RES_NAME);
		sampleObsRes = new SampleObserveResource(SAMPLE_OBS_RES_NAME);
		//sampleProxyRes = new ProxyCoapClientResource(SAMPLE_PROXY_RES_NAME);
		
        coapServer.add(sampleObsRes);
		coapServer.add(sampleRes);
		//coapServer.add(sampleProxyRes);
		
		NetworkConfig config = new NetworkConfig()
		    	.setInt(NetworkConfig.Keys.MAX_MESSAGE_SIZE, 4096)
				.setInt(NetworkConfig.Keys.MAX_MESSAGE_SIZE, 1024)
		    	.setInt(NetworkConfig.Keys.PREFERRED_BLOCK_SIZE, 1024)
		    	.setInt(NetworkConfig.Keys.BLOCKWISE_STATUS_LIFETIME, 100)
		    	.setInt(NetworkConfig.Keys.ACK_TIMEOUT, 500)
		    	.setFloat(NetworkConfig.Keys.ACK_RANDOM_FACTOR, 1f)
		    	.setFloat(NetworkConfig.Keys.ACK_TIMEOUT_SCALE, 2f)
		    	.setInt(NetworkConfig.Keys.MAX_RETRANSMIT, 6);
        
		InMemoryPskStore pskStore = new InMemoryPskStore();

		// put in the PSK store the default identity/psk for tinydtls tests
		pskStore.setKey("Client_identity", "secretPSK".getBytes());
		
		try {
			// load the key store
			SslContextUtil.Credentials serverCredentials = SslContextUtil.loadCredentials(
					SslContextUtil.CLASSPATH_SCHEME + KEY_STORE_LOCATION, "server", KEY_STORE_PASSWORD,
					KEY_STORE_PASSWORD);
			Certificate[] trustedCertificates = SslContextUtil.loadTrustedCertificates(
					SslContextUtil.CLASSPATH_SCHEME + TRUST_STORE_LOCATION, "root", TRUST_STORE_PASSWORD);

			DtlsConnectorConfig.Builder builder = new DtlsConnectorConfig.Builder();
			builder.setRecommendedCipherSuitesOnly(false);
			builder.setAddress(new InetSocketAddress(DEFAULT_PORT));
			builder.setPskStore(pskStore);
			builder.setIdentity(serverCredentials.getPrivateKey(), serverCredentials.getCertificateChain(),
					CertificateType.RAW_PUBLIC_KEY, CertificateType.X_509);
			builder.setTrustStore(trustedCertificates);
			builder.setClientAuthenticationRequired(false);
			dtlsConnector = new DTLSConnector(builder.build());
			
			CoapEndpoint.Builder coapsBuilder = new CoapEndpoint.Builder();
			coapsBuilder.setConnector(dtlsConnector);
			coapsBuilder.setNetworkConfig(config);
			
			CoapEndpoint.Builder coapBuilder = new CoapEndpoint.Builder();
			coapBuilder.setPort(5683);
			coapBuilder.setNetworkConfig(config);

			coapServer.addEndpoint(coapBuilder.build());
			coapServer.addEndpoint(coapsBuilder.build());
		} catch (GeneralSecurityException | IOException e) {
			System.out.println("Could not load the keystore" + e);
		}
	}
	
	public void start() {
		if (coapServer != null) {
			coapServer.start();
		}
	}
	
	public void stop() {
		if (coapServer != null) {
			coapServer.destroy();
		}
	}
	
	public static void displayInformation()
	{
		System.out.println("************************************************************");
		System.out.println("* CoAP Server");
		System.out.println("* ver. " + VERSION);
		System.out.println("* resources");
		System.out.println("* 	\\" + SAMPLE_RES_NAME);
		System.out.println("* 	\\" + SAMPLE_OBS_RES_NAME);
		//System.out.println("* 	\\" + SAMPLE_PROXY_RES_NAME);
		System.out.println("************************************************************");
		System.out.println("");
	}

	public static void main(String[] args) {
		CoapServerSample server = new CoapServerSample();
		
		displayInformation();
		
		server.start();
	}
}
