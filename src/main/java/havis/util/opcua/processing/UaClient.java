package havis.util.opcua.processing;

import java.rmi.RemoteException;
import java.util.Map;
import java.util.Observable;
import java.util.Observer;
import java.util.logging.Level;
import java.util.logging.Logger;

import havis.util.opcua.DataProvider;
import havis.util.opcua.OPCUAException;

public class UaClient implements UaConnector {
	private UaRemote opcua;
	private UaProcess process;
	private Observable current;
	private DataProviderRemoteObject dataProviderRemoteObject;
	private Map<String, String> properties;

	private final static Logger log = Logger.getLogger(UaClient.class.getName());

	private void startProcess() {
		this.current = this.process.get();
		this.current.addObserver(new Observer() {
			@Override
			public void update(Observable o, Object remote) {
				boolean reinit = false;
				if (opcua != null) {
					reinit = true;
				}
				opcua = (UaRemote) remote;
				if (reinit) {
					try {
						log.info("Re-Init OPC UA connection due to errors");
						opcua.open(properties, dataProviderRemoteObject);
					} catch (Exception e) {
						log.log(Level.SEVERE, "Could not re-open OPC-UA object", e);
					}
				}
			}
		});
	}

	private void stopProcess() {
		if (this.current != null) {
			this.process.unget(this.current);
		}
	}

	public UaClient(UaProcess process) throws RemoteException {
		this.process = process;
		startProcess();

	}

	public void open(Map<String, String> properties, DataProvider handler) throws OPCUAException {
		try {
			this.properties = properties;
			this.dataProviderRemoteObject = new DataProviderRemoteObject(handler);
			opcua.open(properties, dataProviderRemoteObject);
		} catch (RemoteException e) {
			throw new OPCUAException("Remote connection was lost to server process of OPCUA server: " + e.toString());
		}
	}

	public void close() throws OPCUAException {
		try {
			stopProcess();
		} catch (Exception e) {
			throw new OPCUAException("Remote connection was lost to server process of OPCUA server: " + e.toString());
		}
	}

	@Override
	public void notification(int namespace, Object nodeId, Object obj) throws OPCUAException {
		try {
			opcua.notification(namespace, nodeId, obj);
		} catch (RemoteException e) {
			throw new OPCUAException("Remote connection was lost to server process of OPCUA server: " + e.toString());
		}

	}

	@Override
	public void event(int namespace, Object eventId, int paramNamespace, Object paramId, long timestamp, int severity, String msg, Object obj) throws OPCUAException {
		try {
			opcua.event(namespace, eventId, paramNamespace, paramId, timestamp, severity, msg, obj);
		} catch (RemoteException e) {
			throw new OPCUAException("Remote connection was lost to server process of OPCUA server: " + e.toString());
		}

	}

}
