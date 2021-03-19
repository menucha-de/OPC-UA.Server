package havis.util.opcua;

import java.io.Serializable;

public interface DataProvider extends Serializable{	
	Object read(int namespace, Object id) throws Exception;  
	void write(int namespace, Object id, Object value) throws Exception;
	void subscribe(int namespace, Object id) throws Exception;
	void unsubscribe(int namespace, Object id) throws Exception;
	Object exec(int namespaceMethod, Object methodId, int nodeNamespace, Object nodeId, Object params)  throws Exception;
}