package NodePackage;

import java.util.Set;
import java.util.TreeSet;

/**
 * Created by kannabi on 16.12.16.
 */
public class Message {
    private String body;
    private Set<String> addr;

    Message(String body, Set<String> addr){
        this.body = body;
        this.addr = copySet(addr);
    }

    public String getBody(){
        return body;
    }

    public Set<String> getAddr(){
        return addr;
    }

    private Set<String> copySet (Set<String> from){
        Set<String> to = new TreeSet<>();
        for (String aFrom : from)
            to.add(aFrom);
        return to;
    }
}
