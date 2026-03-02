package da16200_coap_server_sample;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.server.resources.CoapExchange;

public class SampleResource extends CoapResource {
	public SampleResource(String name) {
		super(name);
	}
	
	@Override
    public void handleGET(CoapExchange exchange) {
    	System.out.println("Received GET request");
    	
    	String payload = "Sample CoAP Server";
    	
    	exchange.respond(ResponseCode.CONTENT, payload);
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
    	System.out.println("Received POST request");
    	exchange.respond(ResponseCode.CHANGED, exchange.getRequestText());
    }

    @Override
    public void handlePUT(CoapExchange exchange) {
    	System.out.println("Received PUT request");
    	exchange.respond(ResponseCode.CHANGED);
    }
    
    @Override
    public void handleDELETE(CoapExchange exchange) {
    	System.out.println("Received DELETE request");
		exchange.respond(ResponseCode.DELETED);
	}
}