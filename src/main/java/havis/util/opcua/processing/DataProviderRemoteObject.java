package havis.util.opcua.processing;

import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.Objects;

import havis.util.opcua.DataProvider;

public class DataProviderRemoteObject extends UnicastRemoteObject implements DataProviderRemote {

	private DataProvider messageHandler;

	protected DataProviderRemoteObject(DataProvider messageHandler) throws RemoteException {
		super();
		this.messageHandler = Objects.requireNonNull(messageHandler, "messaqeHandler must not be null");
	}

	private static final long serialVersionUID = 1L;

	@Override
	public void write(int namespace, Object id, Object value) throws RemoteException {
		try {
			messageHandler.write(namespace, id, value);
		} catch (Exception e) {
			throw new RemoteException("", e);
		}
	}

	@Override
	public void unsubscribe(int namespace, Object id) throws RemoteException {
		try {
			messageHandler.unsubscribe(namespace, id);
		} catch (Exception e) {
			throw new RemoteException("", e);
		}

	}

	@Override
	public void subscribe(int namespace, Object id) throws RemoteException {
		try {
			messageHandler.subscribe(namespace, id);
		} catch (Exception e) {
			throw new RemoteException("", e);
		}

	}

	@Override
	public Object read(int namespace, Object id) throws RemoteException {
		try {
			return messageHandler.read(namespace, id);
		} catch (Exception e) {
			throw new RemoteException("", e);
		}
	}

	@Override
	public Object exec(int methodNs, Object methodId, int objectNs, Object paramId, Object params) throws RemoteException {
		try {
			return messageHandler.exec(methodNs, methodId, objectNs , paramId, params);
		} catch (Exception e) {
			throw new RemoteException("", e);
		}
	}
}
