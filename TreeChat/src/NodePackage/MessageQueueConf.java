package NodePackage;

import ConstPackage.Const;

import java.util.*;

/**
 * Created by kannabi on 15.12.16.
 */


public class MessageQueueConf {

    //message (first argument of outer table) -- unique id of some message
    //address (first argument of nested table) -- address that should confirm
    //second arg of nested table -- number of try. How many times we have tried send message to it.
    private Hashtable<String, Hashtable<String, Integer>> queue = new Hashtable<>();
    private Hashtable<String, String> messages = new Hashtable<>();

    private synchronized boolean checkRotten(String id){
        for (String it : queue.get(id).keySet())
            if(queue.get(id).get(it) > Const.MAX_NUM_OF_TRY)
                return true;
        return false;
    }

    private synchronized void new37(){
        for (String it : queue.keySet())
            if(checkRotten(it))
                deleteMessage(it);
    }

    public synchronized void addMessage(String messageUuid, Set<String> addresses, String message){
        if(addresses.isEmpty())
            return;

        Hashtable<String, Integer> addr = new Hashtable<>();
        messages.put(messageUuid, message);

        for(String it : addresses){
            addr.put(it, 0);
        }
        queue.put(messageUuid, addr);

//        System.out.println("---------------------------------------");
//        System.out.println(messageUuid);
//        for (String it : addresses)
//            System.out.println(it);
//        System.out.println(message);
//        System.out.println("---------");
//        for (String it : queue.keySet()){
//            System.out.println(it);
//            System.out.println(messages.get(it));
//            for (String it1 : queue.get(it).keySet())
//                System.out.println(it1);
//        }
//        System.out.println("---------------------------------------");
    }

    public synchronized void deleteMessage(String message){
        if(queue.containsKey(message)){
            queue.remove(message);
            messages.remove(message);
        }
    }

    public synchronized void confirm (String message, String address){
//        System.out.println("CONFIRM");
        if(!queue.containsKey(message))
            return;

        if(queue.get(message).containsKey(address)) {
           queue.get(message).remove(address);
//            System.out.println("rem");
        }
        if(queue.get(message).keySet().isEmpty()){
//            System.out.println("del after confirm");
            deleteMessage(message);
        }
    }

    public synchronized Set<String> getUnconfirmedAddresses (String message){

        new37();

        if(queue.isEmpty())
            return null;

        for (String it : queue.get(message).keySet())
            queue.get(message).put(it, queue.get(message).get(it) + 1);

        return queue.get(message).keySet();
    }

    public synchronized int getNumOfTry(String message, String addr){
        return queue.get(message).get(addr);
    }

    public synchronized Set<String> getMessagesID(){
        return queue.keySet();
    }

    public synchronized String getMessage(String ID){
        return messages.get(ID);
    }

    public synchronized Set<String> getMessagesSet(){
        return queue.keySet();
    }

    public synchronized boolean isEmpty(){
        new37();
        return queue.isEmpty();
    }

    public synchronized boolean isConfirmed(String message){
        return queue.get(message).isEmpty();
    }
}
