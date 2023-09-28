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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.GeneralSecurityException;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPrivateCrtKeySpec;
import java.util.Vector;

import org.bouncycastle.asn1.ASN1ObjectIdentifier;
import org.bouncycastle.asn1.edec.EdECObjectIdentifiers;
import org.bouncycastle.asn1.pkcs.PKCSObjectIdentifiers;
import org.bouncycastle.asn1.pkcs.PrivateKeyInfo;
import org.bouncycastle.asn1.pkcs.RSAPrivateKey;
import org.bouncycastle.asn1.sec.ECPrivateKey;
import org.bouncycastle.asn1.x509.AlgorithmIdentifier;
import org.bouncycastle.asn1.x9.X9ObjectIdentifiers;
import org.bouncycastle.crypto.digests.SHA256Digest;
import org.bouncycastle.crypto.params.AsymmetricKeyParameter;
import org.bouncycastle.crypto.params.RSAPrivateCrtKeyParameters;
import org.bouncycastle.crypto.util.PrivateKeyFactory;
import org.bouncycastle.tls.AlertDescription;
import org.bouncycastle.tls.Certificate;
import org.bouncycastle.tls.SignatureAlgorithm;
import org.bouncycastle.tls.SignatureAndHashAlgorithm;
import org.bouncycastle.tls.TlsContext;
import org.bouncycastle.tls.TlsCredentialedSigner;
import org.bouncycastle.tls.TlsFatalAlert;
import org.bouncycastle.tls.TlsUtils;
import org.bouncycastle.tls.crypto.TlsCertificate;
import org.bouncycastle.tls.crypto.TlsCrypto;
import org.bouncycastle.tls.crypto.TlsCryptoParameters;
import org.bouncycastle.tls.crypto.impl.bc.BcDefaultTlsCredentialedSigner;
import org.bouncycastle.tls.crypto.impl.bc.BcTlsCrypto;
import org.bouncycastle.tls.crypto.impl.jcajce.JcaDefaultTlsCredentialedSigner;
import org.bouncycastle.tls.crypto.impl.jcajce.JcaTlsCrypto;
import org.bouncycastle.util.encoders.Hex;
import org.bouncycastle.util.io.pem.PemObject;
import org.bouncycastle.util.io.pem.PemReader;

public class SimpleTlsServerUtils {
	
	static private String caResourceName = "../cert.pem";
	static private String certResourceName = "../cert.pem";
	static private String keyResourceName = "../private_key.pem";
	
	static private String certResource =
			"-----BEGIN CERTIFICATE-----\r\n" + 
			"MIIDBjCCAe4CCQCg5xtL5ap/CDANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n" + 
			"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n" + 
			"cyBQdHkgTHRkMB4XDTE4MDgyMTA3Mzg0M1oXDTI4MDgxODA3Mzg0M1owRTELMAkG\r\n" + 
			"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n" + 
			"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n" + 
			"ALMgpDjh0c0ZThAxE/B2mBcDAp2KUaeoXqY5+03bZWRQ7McE0og3DV9u14FGwhDQ\r\n" + 
			"J93itsHXnWGZ6Zh1xtPJzoMiaxxaAe+1WdognwlTD8LUr7qS/7VtFXXmvVaIQ7E5\r\n" + 
			"AKLxlFsgVc3soSLo4f06DWOGl7pSIC2EttYFKMvBmlWcksEA1+DXP/NJy8h3wqTF\r\n" + 
			"MC+iydE5cXd3lKJ3CQ+0zTJli4DOF5ZudbrkWOQXwfZ1ylB24umvbNMSfltlSkuC\r\n" + 
			"km0VhF8VCLFwek+WNcrqeDkAAyMyRyVlDi+9r1qTHPF+EYwcJHWh+zFegztGc1w6\r\n" + 
			"7mxBBh+JRBH7GzWLT9kBg7kCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAWJVjI3po\r\n" + 
			"zjCpsfrPw1u/K/0AJGRpLChvBPMYz11/Ay2I2vyT8akRM+7km0W/d+wiyL0YT9QI\r\n" + 
			"PHT4gLEV4KeiQSAPindtJgbg2Wi53EBGeNrDWHMwH3islDr9h293zhUqZMie2WDb\r\n" + 
			"kpEcSh79DAhZDZ43d/qHgWeyWWkyC8JLQIzYPsdTJYf8qq7yNZlr7OZCHXdq2sG8\r\n" + 
			"SjE4I1j4ANkkJpnX6yFDZZGDDrUzPcJRPVtZzmytJUIbzoTU9E6nSDIlvBTT273o\r\n" + 
			"9gJXvkU7L4cwNCAg0g3RRwyqIUq8zyWsIeNSdqz64EIH2wX4XvWkVzDzfxW76HL7\r\n" + 
			"NOds4a5SIHfFZw==\r\n" + 
			"-----END CERTIFICATE-----\r\n";
	static private String keyResource =
			"-----BEGIN RSA PRIVATE KEY-----\r\n" + 
			"MIIEpAIBAAKCAQEAsyCkOOHRzRlOEDET8HaYFwMCnYpRp6hepjn7TdtlZFDsxwTS\r\n" + 
			"iDcNX27XgUbCENAn3eK2wdedYZnpmHXG08nOgyJrHFoB77VZ2iCfCVMPwtSvupL/\r\n" + 
			"tW0Vdea9VohDsTkAovGUWyBVzeyhIujh/ToNY4aXulIgLYS21gUoy8GaVZySwQDX\r\n" + 
			"4Nc/80nLyHfCpMUwL6LJ0Tlxd3eUoncJD7TNMmWLgM4Xlm51uuRY5BfB9nXKUHbi\r\n" + 
			"6a9s0xJ+W2VKS4KSbRWEXxUIsXB6T5Y1yup4OQADIzJHJWUOL72vWpMc8X4RjBwk\r\n" + 
			"daH7MV6DO0ZzXDrubEEGH4lEEfsbNYtP2QGDuQIDAQABAoIBADniYG8pOhznAnzk\r\n" + 
			"/yaDjF5TULMMEZr2I6/fqL/eGAO0yu79NfNipuWh8e4KqYe5XEitjJVTUb5KeFwW\r\n" + 
			"IywpWJyzsJ020M1fcyuzwvDGcJ9rD2ZhPlSobXjuGV0vJ4DLhNMi8egIqPGkd+XK\r\n" + 
			"D80+xzjUM4+4HkHXUyYSAL7nTzI+nh9mm3yCntFSJsNj1tRX7IVjnjh1RIS7MR5B\r\n" + 
			"f0u9xVGf3UbBveK/RyVGd/QWJacktBcr3WyCxHkwo64TETuj5D3NxqIrmiChBauB\r\n" + 
			"ediqoe3RuxPw6vgE3+z3mfDdD0Xb58CsvN27dW//euf1xZjjw6xLqAo3Zd8fAMME\r\n" + 
			"EFDURiECgYEA1mX/Z0OoNId3SoFhg5NkQlEBSb2Lna35JtqhuylPLarjapoGj3C/\r\n" + 
			"4z0hAIU6d1W3L12P175XaHrDuA51Tc9bnMp/JuSrV6r5CvmZJ81MftYlJzl41zJd\r\n" + 
			"JIf1nILxlXvQS9rVHSqIO/0dPwEAkGKJHqqhyWTM5sh3CcgpQy7JsL0CgYEA1eKZ\r\n" + 
			"pDdgkLRQgmtkW0FDIkR1aG6J6gE/TpmmSY64xqEYr5rnGRObvYrGQXHp9oD/QuCH\r\n" + 
			"fiVeudvuyQYXt0KsEv0NyhoLWmAlTij1eKEjUmvHrTV3ydMSdEIs5p5k6w3Ht8KU\r\n" + 
			"4YAjjFglL+0xDV9EC6nwB4swNwKfBpifuwkspK0CgYB1RFjMHJ92C9pdsCKsGwQt\r\n" + 
			"ma0ArmIdHrk2XUM04cVjDyNQfWq1LlBmdFsGs9hkyUdm6t/wezXH+c3vcEkNBCvx\r\n" + 
			"uHiPx2dIjkWlkRwKPypl/a9YowDLg8qaXpsiviRxRMWLl+gVCdx2I13JxjyOvLaP\r\n" + 
			"RXk0dKP2XxNtEEQxcPf0aQKBgQDHXzzcqIopGQvbJoQb1E/iB3Jx8Gg6awM6H1u0\r\n" + 
			"QYfYD57VQk2dQHvySQPZSXhPwZswGd/zJJ6SHYMOe9FrkIiaAqzx8SkYC3t6yg9X\r\n" + 
			"bM1iLPmqaabJySjwmicEqi1kNiovDwB821dHoXq4nB8XWfAx9yy5u3MsNBNMsMRk\r\n" + 
			"Mn8c2QKBgQC0OKboeUgzk9i06KE6CqGO43WpPDKNG+4ylnCnIxjeGIYLNKHJ+RCv\r\n" + 
			"4lt+FHWcwciM3JoNJWSfqXGcqp2B97I/YDW87L6He4ShUmzvdlAG1CecGqTYBrYB\r\n" + 
			"lcr/+RukWzIMUCA24KeAoheKsC+HJGg4U08nWz5n9K30kqNya+nz9g==\r\n" + 
			"-----END RSA PRIVATE KEY-----\r\n";

	static String fingerprint(org.bouncycastle.asn1.x509.Certificate c) throws IOException
	{
		byte[] der = c.getEncoded();
		byte[] sha1 = sha256DigestOf(der);
		byte[] hexBytes = Hex.encode(sha1);
		String hex = new String(hexBytes, "ASCII").toUpperCase();
		
		StringBuffer fp = new StringBuffer();
		int i = 0;
		fp.append(hex.substring(i, i + 2));
		while ((i += 2) < hex.length())
		{
			fp.append(':');
			fp.append(hex.substring(i, i + 2));
		}
		return fp.toString();
	}

    static byte[] sha256DigestOf(byte[] input)
    {
		SHA256Digest d = new SHA256Digest();
		d.update(input, 0, input.length);
		byte[] result = new byte[d.getDigestSize()];
		d.doFinal(result, 0);
		return result;
    }

    static String getCACertResource(short signatureAlgorithm) throws IOException
    {
    	return caResourceName;
    }
    
    static String getResourceName(short signatureAlgorithm) throws IOException
    {
        switch (signatureAlgorithm)
        {
        case SignatureAlgorithm.rsa:
        case SignatureAlgorithm.rsa_pss_rsae_sha256:
        case SignatureAlgorithm.rsa_pss_rsae_sha384:
        case SignatureAlgorithm.rsa_pss_rsae_sha512:
            return "rsa";
        case SignatureAlgorithm.dsa:
            return "dsa";
        case SignatureAlgorithm.ecdsa:
            return "ecdsa";
        case SignatureAlgorithm.ed25519:
            return "ed25519";
        case SignatureAlgorithm.ed448:
            return "ed448";
        case SignatureAlgorithm.rsa_pss_pss_sha256:
            return "rsa_pss_256";
        case SignatureAlgorithm.rsa_pss_pss_sha384:
            return "rsa_pss_384";
        case SignatureAlgorithm.rsa_pss_pss_sha512:
            return "rsa_pss_512";
        default:
            throw new TlsFatalAlert(AlertDescription.internal_error);
        }
    }
    
    static TlsCredentialedSigner loadSignerCredentials(TlsContext context, String[] certResources, String keyResource, SignatureAndHashAlgorithm signatureAndHashAlgorithm) throws IOException
    {
        TlsCrypto crypto = context.getCrypto();
        Certificate certificate = loadCertificateChain(crypto, certResources);
        TlsCryptoParameters cryptoParams = new TlsCryptoParameters(context);

        // TODO[tls-ops] Need to have TlsCrypto construct the credentials from the certs/key (as raw data)
        if (crypto instanceof BcTlsCrypto)
        {
            AsymmetricKeyParameter privateKey = loadBcPrivateKeyResource(keyResource);

            return new BcDefaultTlsCredentialedSigner(cryptoParams, (BcTlsCrypto)crypto, privateKey, certificate, signatureAndHashAlgorithm);
        }
        else
        {
            JcaTlsCrypto jcaCrypto = (JcaTlsCrypto)crypto;
            PrivateKey privateKey = loadJcaPrivateKeyResource(jcaCrypto, keyResource);

            return new JcaDefaultTlsCredentialedSigner(cryptoParams, jcaCrypto, privateKey, certificate, signatureAndHashAlgorithm);
        }
    }
    
    static TlsCredentialedSigner loadSignerCredentials(TlsContext context, Vector supportedSignatureAlgorithms, short signatureAlgorithm, String certResource, String keyResource) throws IOException
    {
        SignatureAndHashAlgorithm signatureAndHashAlgorithm = null;
        if (supportedSignatureAlgorithms == null)
        {
            supportedSignatureAlgorithms = TlsUtils.getDefaultSignatureAlgorithms(signatureAlgorithm);
        }

        for (int i = 0; i < supportedSignatureAlgorithms.size(); ++i)
        {
            SignatureAndHashAlgorithm alg = (SignatureAndHashAlgorithm)
                supportedSignatureAlgorithms.elementAt(i);
            if (alg.getSignature() == signatureAlgorithm)
            {
                // Just grab the first one we find
                signatureAndHashAlgorithm = alg;
                break;
            }
        }

        if (signatureAndHashAlgorithm == null)
        {
            return null;
        }

        return loadSignerCredentials(context, new String[]{ certResource, getCACertResource(signatureAlgorithm) },
            keyResource, signatureAndHashAlgorithm);
    }
    
    static TlsCredentialedSigner loadSignerCredentialsServer(TlsContext context, Vector supportedSignatureAlgorithms, short signatureAlgorithm) throws IOException
    {
		return loadSignerCredentials(context, supportedSignatureAlgorithms, signatureAlgorithm, certResourceName, keyResourceName);
    }

    static Certificate loadCertificateChain(TlsCrypto crypto, String[] resources) throws IOException
    {
        TlsCertificate[] chain = new TlsCertificate[resources.length];
        for (int i = 0; i < resources.length; ++i)
        {
            chain[i] = loadCertificateResource(crypto, resources[i]);
        }
        return new Certificate(chain);
    }

    static TlsCertificate loadCertificateResource(TlsCrypto crypto, String resource) throws IOException
    {
        PemObject pem = loadPemResource(resource);
        if (pem.getType().endsWith("CERTIFICATE"))
        {
            return crypto.createCertificate(pem.getContent());
        }
        throw new IllegalArgumentException("'resource' doesn't specify a valid certificate");
    }

    static AsymmetricKeyParameter loadBcPrivateKeyResource(String resource) throws IOException
    {
        PemObject pem = loadPemResource(resource);
        if (pem.getType().equals("PRIVATE KEY"))
        {
            return PrivateKeyFactory.createKey(pem.getContent());
        }
        if (pem.getType().equals("ENCRYPTED PRIVATE KEY"))
        {
            throw new UnsupportedOperationException("Encrypted PKCS#8 keys not supported");
        }
        if (pem.getType().equals("RSA PRIVATE KEY"))
        {
            RSAPrivateKey rsa = RSAPrivateKey.getInstance(pem.getContent());
            return new RSAPrivateCrtKeyParameters(rsa.getModulus(), rsa.getPublicExponent(),
                rsa.getPrivateExponent(), rsa.getPrime1(), rsa.getPrime2(), rsa.getExponent1(),
                rsa.getExponent2(), rsa.getCoefficient());
        }
        if (pem.getType().equals("EC PRIVATE KEY"))
        {
            ECPrivateKey pKey = ECPrivateKey.getInstance(pem.getContent());
            AlgorithmIdentifier algId = new AlgorithmIdentifier(X9ObjectIdentifiers.id_ecPublicKey, pKey.getParameters());
            PrivateKeyInfo privInfo = new PrivateKeyInfo(algId, pKey);
            return PrivateKeyFactory.createKey(privInfo);
        }
        throw new IllegalArgumentException("'resource' doesn't specify a valid private key");
    }

    static PrivateKey loadJcaPrivateKeyResource(JcaTlsCrypto crypto, String resource) throws IOException
    {
        Throwable cause = null;
        try
        {
            PemObject pem = loadPemResource(resource);
            if (pem.getType().equals("PRIVATE KEY"))
            {
                return loadJcaPkcs8PrivateKey(crypto, pem.getContent());
            }
            if (pem.getType().equals("ENCRYPTED PRIVATE KEY"))
            {
                throw new UnsupportedOperationException("Encrypted PKCS#8 keys not supported");
            }
            if (pem.getType().equals("RSA PRIVATE KEY"))
            {
                RSAPrivateKey rsa = RSAPrivateKey.getInstance(pem.getContent());
                KeyFactory keyFact = crypto.getHelper().createKeyFactory("RSA");
                return keyFact.generatePrivate(new RSAPrivateCrtKeySpec(rsa.getModulus(), rsa.getPublicExponent(),
                    rsa.getPrivateExponent(), rsa.getPrime1(), rsa.getPrime2(), rsa.getExponent1(), rsa.getExponent2(),
                    rsa.getCoefficient()));
            }
        }
        catch (GeneralSecurityException e)
        {
            cause = e;
        }
        throw new IllegalArgumentException("'resource' doesn't specify a valid private key", cause);
    }

    static PrivateKey loadJcaPkcs8PrivateKey(JcaTlsCrypto crypto, byte[] encoded) throws GeneralSecurityException
    {
        PrivateKeyInfo pki = PrivateKeyInfo.getInstance(encoded);
        AlgorithmIdentifier algID = pki.getPrivateKeyAlgorithm();
        ASN1ObjectIdentifier oid = algID.getAlgorithm();

        String name;
        if (X9ObjectIdentifiers.id_dsa.equals(oid))
        {
            name = "DSA";
        }
        else if (X9ObjectIdentifiers.id_ecPublicKey.equals(oid))
        {
            // TODO Try ECDH/ECDSA according to intended use?
            name = "EC";
        }
        else if (PKCSObjectIdentifiers.rsaEncryption.equals(oid)
            || PKCSObjectIdentifiers.id_RSASSA_PSS.equals(oid))
        {
            name = "RSA";
        }
        else if (EdECObjectIdentifiers.id_Ed25519.equals(oid))
        {
            name = "Ed25519";
        }
        else if (EdECObjectIdentifiers.id_Ed448.equals(oid))
        {
            name = "Ed448";
        }
        else
        {
            name = oid.getId();
        }

        KeyFactory kf = crypto.getHelper().createKeyFactory(name);
        return kf.generatePrivate(new PKCS8EncodedKeySpec(encoded));
    }

    static PemObject loadPemResource(String resource) throws IOException
    {
    	InputStream s = null;
        
        if (resource.equals(caResourceName) || resource.equals(certResourceName)) {
        	s = new ByteArrayInputStream(certResource.getBytes());
        } else if (resource.equals(keyResourceName)) {
        	s = new ByteArrayInputStream(keyResource.getBytes());
        } else {
        	s = SimpleTlsServerUtils.class.getResourceAsStream(resource);
        }
         
        PemReader p = new PemReader(new InputStreamReader(s));
        PemObject o = p.readPemObject();
        p.close();
        return o;
    }
}
