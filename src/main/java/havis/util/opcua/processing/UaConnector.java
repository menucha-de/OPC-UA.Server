package havis.util.opcua.processing;

import java.util.Map;

import havis.util.opcua.DataProvider;
import havis.util.opcua.OPCUAException;

public interface UaConnector {
	public void open(Map<String, String> properties, DataProvider handler) throws OPCUAException;
	void notification(int namespace, Object nodeId, Object obj) throws OPCUAException;
	void event(int namespace, Object eventId, int paramNamespace, Object paramId, long timestamp, int severity, String msg, Object obj) throws OPCUAException;
}
