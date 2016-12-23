package NodePackage;

import java.net.InetAddress;

public class Connection {
    private InetAddress addr;
    private int port;
    private boolean connected;

    Connection(InetAddress addr, int port) {
        this.addr = addr;
        this.port = port;
        this.connected = true;
    }

    public InetAddress getAddr(){
        return addr;
    }

    public int getPort(){
        return port;
    }

    public boolean equals (Connection connection){
        return this.addr.equals(connection.addr) && (this.port == connection.port);
    }
}
