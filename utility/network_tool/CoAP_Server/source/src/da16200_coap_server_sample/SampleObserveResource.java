package da16200_coap_server_sample;

import java.util.Timer;
import java.util.TimerTask;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.CoAP.Type;
import org.eclipse.californium.core.server.resources.CoapExchange;

public class SampleObserveResource extends CoapResource {
	
	private int max_age = 10;	//sec
	
	Timer observe_notification_timer;
	
	private class NotificationTask extends TimerTask {
		@Override
		public void run() {
			System.out.println("Send Observe notification");
			changed();
			this.cancel();
		}
	}
	
	public void start_periodic_notify(int sec) {
		if (observe_notification_timer != null) {
			observe_notification_timer.cancel();
		}
		
		observe_notification_timer = new Timer();
		
		observe_notification_timer.schedule(new NotificationTask(), (sec - 5) * 1000, (sec - 5) * 1000);
	}
	
	public SampleObserveResource(String name) {
		super(name);
		
		setObservable(true);				//enable observing
		setObserveType(Type.CON);			//configure the notification type to CONs
		getAttributes().setObservable();	//mark observable in the Link Format
	}
	
    @Override
    public void handleGET(CoapExchange exchange) {
    	String msg = "CoAP Observe Notification";
    	
    	exchange.setMaxAge(max_age);
    	
    	exchange.respond(ResponseCode.CONTENT, msg);
    	
    	if (getObserverCount() > 0) {
    		start_periodic_notify(max_age);	
    	}
    }
}