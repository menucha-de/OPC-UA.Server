package havis.util.opcua;

import java.io.Serializable;

public interface MessageHandler extends Serializable {
    void valueChanged(Object id, Object params);
    void usabilityChanged(Object var, boolean isUsable);
    void messageReceived(Object var);
}
