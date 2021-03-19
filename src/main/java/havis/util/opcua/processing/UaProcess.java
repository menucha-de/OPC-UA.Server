package havis.util.opcua.processing;

import java.util.Observable;

public interface UaProcess {

    /**
    * @return the observable which retrieves the remote access
    */
    Observable get();

    /**
    * Stop the process
    */
    void unget(Observable observable);
}

