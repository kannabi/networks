package NodePackage;

import java.nio.channels.SelectionKey;
import java.util.Set;
import java.util.TreeSet;

/**
 * Created by kannabi on 16.12.16.
 */
public class Message {
    private String body;
    private String uuid;
    private Set<String> addr;

    Message(String body, Set<String> addr){
        this.body = body;
        this.addr = copySet(addr);
    }

    Message(String body, Set<String> addr, String uuid){
        this.body = body;
        this.addr = copySet(addr);
        this.uuid = uuid;
    }

    public String getBody(){
        return body;
    }

    public Set<String> getAddr(){
        return addr;
    }

    public String getUuid(){
        return uuid;
    }

    public boolean removeAddr(String address){
        return addr.remove(address);
    }

    public boolean isAddrEmpty(){
        return addr.isEmpty();
    }

    private Set<String> copySet (Set<String> from){
        Set<String> to = new TreeSet<>();
        for (String aFrom : from)
            to.add(aFrom);
        return to;
    }
}
