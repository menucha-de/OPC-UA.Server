package havis.util.opcua;

public class OPCUAException extends Exception {

	private int statusCode=0;
	
	private static final long serialVersionUID = 1L;
	
	public OPCUAException(String message, Throwable cause) {
		super(message, cause);
	}

	public OPCUAException(Throwable cause) {
		super(cause);
	}

	public OPCUAException(String message) {
		super(message);
	}
	
	
	public OPCUAException(int code, String message) {
		super(message);
		statusCode = code;
	}
	
	public int getStatusCode() {
		return statusCode;
	}
	

}
