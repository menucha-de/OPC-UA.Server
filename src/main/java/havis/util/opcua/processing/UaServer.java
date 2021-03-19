package havis.util.opcua.processing;

import java.rmi.RemoteException;
import java.util.Map;

import havis.util.opcua.DataProvider;
import havis.util.opcua.OPCUADataProvider;

public class UaServer implements UaRemote {

	// private static final Logger log =
	// Logger.getLogger(AimServer.class.getName());

	OPCUADataProvider worker = new OPCUADataProvider();

	@Override
	public void open(Map<String, String> properties, final DataProviderRemote handler) throws RemoteException {
		try {
			worker.open(properties, new DataProvider() {
				/**
				 * 
				 */
				private static final long serialVersionUID = 1L;

				@Override
				public void write(int namespace, Object id, Object value) throws Exception {
					handler.write(namespace, id, value);
				}

				@Override
				public void unsubscribe(int namespace, Object id) throws Exception {
					handler.unsubscribe(namespace, id);
				}

				@Override
				public void subscribe(int namespace, Object id) throws Exception {
					handler.subscribe(namespace, id);
				}

				@Override
				public Object read(int namespace, Object id) throws Exception {
					return handler.read(namespace, id);
				}

				@Override
				public Object exec(int methodNs, Object methodId, int objectNs, Object paramId, Object params) throws Exception {
					return handler.exec(methodNs, methodId, objectNs, paramId, params);
				}
			});
		} catch (Exception e) {
			throw new RemoteException("Failed to open server", e);
		}

	}

	@Override
	public void notification(int namespace, Object nodeId, Object obj) throws RemoteException {
		worker.notification(namespace, nodeId, obj);
	}

	@Override
	public void event(int namespace, Object eventId, int paramNamespace, Object paramId, long timestamp, int severity, String msg, Object obj) throws RemoteException {
		worker.event(namespace, eventId, paramNamespace, paramId, timestamp, severity, msg, obj);

	}

}
