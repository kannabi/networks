package NodePackage;

import java.util.Vector;

/**
 * Created by kannabi on 16.12.16.
 */
public class MessageQueue {
    private Vector<Message> queue = new Vector<>();

    private boolean valueSet;

    MessageQueue(){
        valueSet = false;
    }

    public synchronized void put(Message message){
        if(message.getAddr().isEmpty()) {
            return;
        }

        queue.add(message);
    }

    public synchronized Message get(){
        Message mes = queue.lastElement();
        queue.remove(queue.size() - 1);

        return mes;
    }

    public synchronized boolean isEmpty(){
        return queue.isEmpty();
    }
}
