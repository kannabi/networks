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
        while (valueSet)
            try{
                wait();
            } catch (InterruptedException e){
                e.printStackTrace();
            }
        if(message.getAddr().isEmpty()) {
            return;
        }

        queue.add(message);
        valueSet = true;
        notify();
    }

    public synchronized Message get(){
        while(!valueSet)
            try {
                wait();
            } catch (InterruptedException e){
                e.printStackTrace();
            }

//        System.out.println("get");
        Message mes = queue.lastElement();
        queue.remove(queue.size() - 1);
        valueSet = false;
        notify();
        return mes;
    }

    public synchronized boolean isEmpty(){
        return queue.isEmpty();
    }
}
