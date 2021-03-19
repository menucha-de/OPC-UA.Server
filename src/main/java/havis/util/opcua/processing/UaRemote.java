package havis.util.opcua.processing;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.util.Map;

public interface UaRemote extends Remote {

	public void open(Map<String, String> properties, DataProviderRemote provider) throws RemoteException;
	void notification(int namespace, Object nodeId, Object obj) throws RemoteException;
	void event(int namespace, Object eventId, int paramNamespace, Object paramId, long timestamp, int severity, String msg, Object obj) throws RemoteException;
}
