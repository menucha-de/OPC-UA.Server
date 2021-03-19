package havis.util.opcua.processing;

import java.rmi.Remote;
import java.rmi.RemoteException;

public interface DataProviderRemote extends Remote {
	Object read(int namepspace, Object id) throws RemoteException;  
	void write(int namepspace, Object id, Object value) throws RemoteException;
	void subscribe(int namepspace, Object id) throws RemoteException;
	void unsubscribe(int namepspace, Object id) throws RemoteException;
	Object exec(int methodNs, Object methodId, int objectNs, Object paramId, Object params)  throws RemoteException;
}
