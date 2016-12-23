import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;

/**
 * Created by kannabi on 23.12.16.
 */
public class BufferSelectionKey {
    private final int BUFFER_SIZE = 65536;

    private ByteBuffer buffer = ByteBuffer.allocate(BUFFER_SIZE);

    private SelectionKey key;

    boolean inputShut = false;
    boolean outputShut = false;

    BufferSelectionKey(SelectionKey key) {
        this.key = key;
    }

    public ByteBuffer getBuffer() {
        return buffer;
    }

    public SelectionKey getKey() {
        return key;
    }

    public boolean isShut() {
        return inputShut && outputShut;
    }
}
