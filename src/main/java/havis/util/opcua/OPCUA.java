
package havis.util.opcua;

import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

public class OPCUA {

	private final static Logger log = Logger.getLogger(OPCUA.class.getName());

	static {
		try {
			System.loadLibrary("java-opcua");
		} catch (Throwable e) {
			log.log(Level.SEVERE, "Failed to load system library", e);
		}
	}

	public native void open(Map<String, String> properties, MessageHandler handler);

	public native Object read(int namespace, Object nodeId);

	public native Object exec(int objectNamespace, Object objectId, int methodNamespace, Object methodId, Object params);
	
	public native void write(int namespace, Object nodeId, Object params);
	
	public native void subscribe(int namespace, Object nodeId);
	
	public native void unsubscribe(int namespace, Object nodeId);
	
	public native Map<String, Map<String, String>> browse(int namespace, Object nodeId, String prefix);
	
	public native void close ();
	

}
