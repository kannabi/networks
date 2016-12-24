package NodePackage;

import ConstPackage.Const;

import java.nio.channels.SelectionKey;
import java.util.*;

/**
 * Created by kannabi on 15.12.16.
 */


public class MessageQueueConf {

    private final int MAX_LENGTH = 64;

    //message (first argument of outer table) -- unique id of some message
    //address (first argument of nested table) -- address that should confirm
    //second arg of nested table -- number of try. How many times we have tried send message to it.
    private Vector<Message> messages = new Vector<>();

//    private Hashtable<String, Hashtable<String, Integer>> queue = new Hashtable<>();
//    private Hashtable<String, String> messages = new Hashtable<>();

//    private synchronized boolean checkRotten(String id){
//        for (String it : queue.get(id).keySet())
//            if(queue.get(id).get(it) > Const.MAX_NUM_OF_TRY)
//                return true;
//        return false;
//    }
//
//    private synchronized void new37(){
//        for (String it : queue.keySet())
//            if(checkRotten(it))
//                deleteMessage(it);
//    }

    public synchronized void addMessage(String messageUuid, Set<String> addresses, String message){
        if(addresses.isEmpty())
            return;

        if(messages.size() == MAX_LENGTH) {
            messages.remove(0);
        }

        messages.add(new Message(message, addresses, messageUuid));
    }

    public synchronized void confirm (String message, String address){
        System.out.println("CONFIRM");
        int index;

        if((index = findMessage(message)) == Const.EXIT_FAILURE)
            return;

        if(!messages.get(index).removeAddr(address))
            System.out.println("Error: this address already confirmed");

        if(messages.get(index).isAddrEmpty())
            messages.remove(index);
    }

    public synchronized Set<String> getUnconfirmedAddresses (String message){

        if(messages.isEmpty())
            return null;

        return messages.get(findMessage(message)).getAddr();
    }

//    public synchronized int getNumOfTry(String message, String addr){
//        return queue.get(message).get(addr);
//    }

//    public synchronized Set<String> getMessagesID(){
//        return queue.keySet();
//    }

    public synchronized String getMessage(String ID){
        return messages.get(findMessage(ID)).getBody();
    }

    public synchronized Set<String> getMessagesSet(){
        Set<String> keySet = new TreeSet<>();

        for (Message it : messages)
            keySet.add(it.getUuid());

        return keySet;
    }

    public synchronized boolean isEmpty(){
        return messages.isEmpty();
    }

    private int findMessage(String id){
        for(Message it : messages)
            if(it.getUuid().equals(id))
                return messages.indexOf(it);

        return Const.EXIT_FAILURE;
    }

    public synchronized boolean isConfirmed(String message){
        return messages.get(findMessage(message)).isAddrEmpty();
    }
}
