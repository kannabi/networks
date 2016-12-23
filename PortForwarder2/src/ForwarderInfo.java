import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;

/**
 * Created by kannabi on 23.12.16.
 */
public class ForwarderInfo {
    private final int BUFFER_SIZE = 1024;

    private ByteBuffer buffer = ByteBuffer.allocate(BUFFER_SIZE);

    private SelectionKey key;

    private boolean inputShut = false;
    private boolean outputShut = false;

    public void setInputShutted(){
        inputShut = true;
    }

    public void setOutputShutted(){
        outputShut = true;
    }

    public boolean isInputShut(){
        return inputShut;
    }

    public boolean isOutputShut(){
        return outputShut;
    }

    ForwarderInfo(SelectionKey key) {
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
