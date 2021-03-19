package havis.util.opcua;

import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

public class OPCUADataProvider {
	private final static Logger log = Logger.getLogger(OPCUADataProvider.class.getName());
	static {
		try {
			System.loadLibrary("java-opcua-provider");
		} catch (Throwable e) {
			log.log(Level.SEVERE, "Failed to load system library", e);
		}
	}
	public native void open(Map<String, String> properties, DataProvider handler);
	public native void notification(int namespace, Object nodeId, Object value);
	public native void event(int eventNamespace, Object eventId,  int paramNamespace, Object paramId, long timestamp, int severity, String msg, Object param);
}
