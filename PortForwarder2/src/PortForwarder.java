import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;

/**
 * Created by kannabi on 23.12.16.
 */
public class PortForwarder {

    private final int MAX_CONNECTIONS = 64;
    private final int NO_DATA = -1;

    private int localPort;
    private String serverHost;
    private int serverPort;

    InetSocketAddress localAddress;
    InetSocketAddress serverAddress;

    Selector selector;
    ServerSocketChannel listener;

    public PortForwarder(int localPort, String serverHost, int serverPort) {
        this.localPort = localPort;
        this.serverHost = serverHost;
        this.serverPort = serverPort;

        localAddress = new InetSocketAddress(localPort);
        serverAddress = new InetSocketAddress(serverHost, serverPort);

        working();
    }

    private void working() {

        try {
            listener = ServerSocketChannel.open();
            listener.configureBlocking(false);
            listener.bind(localAddress, MAX_CONNECTIONS);

            selector = Selector.open();
            listener.register(selector, SelectionKey.OP_ACCEPT);

            while(selector.select() > NO_DATA){
                for (SelectionKey key : selector.selectedKeys())
                    processKey(key);

                selector.selectedKeys().clear();
            }

        } catch (IOException ie) {
            ie.printStackTrace();
        }
    }

    private void processKey(SelectionKey key){
        if (!key.isValid()) {
            return;
        }

        if (key.isAcceptable()) {
            acceptConnection();
            return;
        }

        if (key.isConnectable())
            completeConnection(key);

        if (key.isWritable())
            writeData(key);

        if (key.isReadable()) {
            readData(key);
        }
    }

    private void acceptConnection(){
        try {
            SocketChannel clientChannel = listener.accept();
            if (clientChannel == null){
                System.out.println("Cannot accept connection");
                return;
            }

            System.out.println("accept " + ((InetSocketAddress) clientChannel.getLocalAddress()).getPort() + " ");
            clientChannel.configureBlocking(false);

            if (!serverAddress.isUnresolved()) {
                SocketChannel serverChannel = SocketChannel.open();
                serverChannel.configureBlocking(false);

                SelectionKey clientKey = clientChannel.register(selector, SelectionKey.OP_READ);
                SelectionKey serverKey = serverChannel.register(selector, SelectionKey.OP_CONNECT);
                clientKey.attach(new ForwarderInfo(serverKey));
                serverKey.attach(new ForwarderInfo(clientKey));

                if (serverChannel.connect(serverAddress))
                    serverKey.interestOps(SelectionKey.OP_READ);

            } else {
                clientChannel.close();
            }
        } catch (IOException e){
            e.printStackTrace();
        }
    }

    private void completeConnection(SelectionKey key){
        SocketChannel fromChannel = (SocketChannel) key.channel();
        ForwarderInfo fromInfo = (ForwarderInfo) key.attachment();
        SelectionKey toKey = fromInfo.getKey();
        SocketChannel toChannel = (SocketChannel) toKey.channel();

        try {
            fromChannel.finishConnect();
            System.out.println("connect " + ((InetSocketAddress) fromChannel.getLocalAddress()).getPort() + " ");
            key.interestOps((key.interestOps() & ~SelectionKey.OP_CONNECT) | SelectionKey.OP_READ);
        } catch (IOException e) {
            e.printStackTrace();
            try {
                fromChannel.close();
                toChannel.close();
            } catch (IOException e_){
                e.printStackTrace();
            }
            key.cancel();
            toKey.cancel();
        }
    }

    private void writeData(SelectionKey key){
        SocketChannel fromChannel = (SocketChannel) key.channel();
        ForwarderInfo fromInfo = (ForwarderInfo) key.attachment();
        SelectionKey toKey = fromInfo.getKey();
        ForwarderInfo toInfo = (ForwarderInfo) toKey.attachment();
        SocketChannel toChannel = (SocketChannel) toKey.channel();
        ByteBuffer writeBuffer = toInfo.getBuffer();

        try {
            writeBuffer.flip();
            int written = fromChannel.write(writeBuffer);
            System.out.println("write " + ((InetSocketAddress) fromChannel.getLocalAddress()).getPort() + " " + written);
            writeBuffer.compact();
            if (writeBuffer.position() == 0) {
                key.interestOps(key.interestOps() & ~SelectionKey.OP_WRITE);
                if (toInfo.isInputShut()) {
                    fromChannel.shutdownOutput();
                    fromInfo.setOutputShutted();
                }
            }

            if (fromInfo.isShut() || toInfo.isShut()) {
                fromChannel.close();
                toChannel.close();
                key.cancel();
                toKey.cancel();
            }
        } catch (IOException ie) {
            key.interestOps(key.interestOps() & ~SelectionKey.OP_WRITE);
            fromInfo.setOutputShutted();
            try {
                fromChannel.shutdownOutput();
                toChannel.shutdownInput();
            } catch (IOException e_){
                e_.printStackTrace();
            }
            toInfo.setInputShutted();
        }
    }

    private void readData(SelectionKey key){
        SocketChannel fromChannel = (SocketChannel) key.channel();
        ForwarderInfo fromInfo = (ForwarderInfo) key.attachment();
        SelectionKey toKey = fromInfo.getKey();
        ForwarderInfo toInfo = (ForwarderInfo) toKey.attachment();
        SocketChannel toChannel = (SocketChannel) toKey.channel();
        ByteBuffer readBuffer = fromInfo.getBuffer();

        try {
            int read = fromChannel.read(readBuffer);
            System.out.println("read " + ((InetSocketAddress) fromChannel.getLocalAddress()).getPort() + " " + read);
            if (read == -1) {
                key.interestOps(key.interestOps() & ~SelectionKey.OP_READ);
                fromChannel.shutdownInput();
                fromInfo.setInputShutted();

                if (readBuffer.position() == 0) {
                    toChannel.shutdownOutput();
                    toInfo.setOutputShutted();
                }
            } else if ((read > 0) || (readBuffer.position() > 0)) {
                toKey.interestOps(toKey.interestOps() | SelectionKey.OP_WRITE);
            }

            if (fromInfo.isShut() || toInfo.isShut()) {
                fromChannel.close();
                toChannel.close();
                key.cancel();
                toKey.cancel();
            }
        } catch (IOException ie) {
            fromInfo.setInputShutted();
            try {
                fromChannel.shutdownInput();
                fromChannel.shutdownOutput();
            } catch (IOException e_){
                e_.printStackTrace();
            }
            fromInfo.setOutputShutted();
        }
    }
}
