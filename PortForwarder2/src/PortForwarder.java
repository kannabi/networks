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

    private int localPort;
    private String serverHost;
    private int serverPort;

    InetSocketAddress localAddress;
    InetSocketAddress serverAddress;

    Selector selector;
    ServerSocketChannel listener;
    Selector selector

    public PortForwarder(int localPort, String serverHost, int serverPort) {
        this.localPort = localPort;
        this.serverHost = serverHost;
        this.serverPort = serverPort;

        localAddress = new InetSocketAddress(localPort);
        serverAddress = new InetSocketAddress(serverHost, serverPort);
    }

    private void working() {

        try {
            listener = ServerSocketChannel.open();
            listener.configureBlocking(false);
            listener.bind(localAddress, MAX_CONNECTIONS);

            selector = Selector.open();
            listener.register(selector, SelectionKey.OP_ACCEPT);

            for (; ; ) {
                if (selector.select() == 0) {
                    continue;
                }

                for (SelectionKey key : selector.selectedKeys()) {
                    if (!key.isValid()) {
                        continue;
                    }

                    if (key.isAcceptable()) {
                        SocketChannel clientChannel = listener.accept();
                        if (clientChannel != null) {
                            System.out.println("accept " + ((InetSocketAddress) clientChannel.getLocalAddress()).getPort() + " ");
                            clientChannel.configureBlocking(false);

                            if (!serverAddress.isUnresolved()) {
                                SocketChannel serverChannel = SocketChannel.open();
                                serverChannel.configureBlocking(false);

                                SelectionKey clientKey = clientChannel.register(selector, SelectionKey.OP_READ);
                                SelectionKey serverKey = serverChannel.register(selector, SelectionKey.OP_CONNECT);
                                clientKey.attach(new BufferSelectionKey(serverKey));
                                serverKey.attach(new BufferSelectionKey(clientKey));

                                if (serverChannel.connect(serverAddress)) {
                                    serverKey.interestOps(SelectionKey.OP_READ);
                                }
                            } else {
                                clientChannel.close();
                            }
                        }
                        continue;
                    }

                    SocketChannel channel = (SocketChannel) key.channel();
                    BufferSelectionKey attachment = (BufferSelectionKey) key.attachment();
                    SelectionKey otherKey = attachment.getKey();
                    BufferSelectionKey otherAttachment = (BufferSelectionKey) otherKey.attachment();
                    SocketChannel otherChannel = (SocketChannel) otherKey.channel();
                    ByteBuffer readBuffer = attachment.getBuffer();
                    ByteBuffer writeBuffer = otherAttachment.getBuffer();

                    if (key.isConnectable()) {
                        try {
                            channel.finishConnect();
                            System.out.println("connect " + ((InetSocketAddress) channel.getLocalAddress()).getPort() + " ");
                            key.interestOps((key.interestOps() & ~SelectionKey.OP_CONNECT) | SelectionKey.OP_READ);
                        } catch (IOException ie) {
                            logError(ie);
                            channel.close();
                            otherChannel.close();
                            key.cancel();
                            otherKey.cancel();
                        }
                    }

                    if (key.isWritable()) {
                        try {
                            writeBuffer.flip();
                            int written = channel.write(writeBuffer);
                            System.out.println("write " + ((InetSocketAddress) channel.getLocalAddress()).getPort() + " " + written);
                            writeBuffer.compact();
                            if (writeBuffer.position() == 0) {
                                key.interestOps(key.interestOps() & ~SelectionKey.OP_WRITE);
                                if (otherAttachment.inputShut) {
                                    channel.shutdownOutput();
                                    attachment.outputShut = true;
                                }
                            }
                        } catch (IOException ie) {
                            key.interestOps(key.interestOps() & ~SelectionKey.OP_WRITE);
                            channel.shutdownOutput();
                            attachment.outputShut = true;
                            otherChannel.shutdownInput();
                            otherAttachment.inputShut = true;
                        }
                    }

                    if (key.isReadable()) {
                        try {
                            int read = channel.read(readBuffer);
                            System.out.println("read " + ((InetSocketAddress) channel.getLocalAddress()).getPort() + " " + read);
                            if (read == -1) {
                                key.interestOps(key.interestOps() & ~SelectionKey.OP_READ);
                                channel.shutdownInput();
                                attachment.inputShut = true;

                                if (readBuffer.position() == 0) {
                                    otherChannel.shutdownOutput();
                                    otherAttachment.outputShut = true;
                                }
                            } else if ((read > 0) || (readBuffer.position() > 0)) {
                                otherKey.interestOps(otherKey.interestOps() | SelectionKey.OP_WRITE);
                            }
                        } catch (IOException ie) {
                            channel.shutdownInput();
                            attachment.inputShut = true;
                            channel.shutdownOutput();
                            attachment.outputShut = true;
                        }
                    }

                    if (attachment.isShut() || otherAttachment.isShut()) {
                        channel.close();
                        otherChannel.close();
                        key.cancel();
                        otherKey.cancel();
                    }
					/* */
                }
                selector.selectedKeys().clear();
				/* */
            }
        } catch (IOException ie) {
            logError(ie);
        }
    }

    private static void logError(Exception exception) {
        System.err.println("Error: " + exception);
    }
}
