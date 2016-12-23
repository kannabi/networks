package NodePackage;

import java.util.Vector;

/**
 * Created by kannabi on 17.12.16.
 */

public class InputMessageCash {
    private final int MAX_SIZE = 64;
    Vector<String> id = new Vector<>();

    public void add(String mesId){
        if(id.size() == MAX_SIZE)
            id.remove(0);
        id.add(mesId);
    }

    public boolean contain(String mesId){
        return id.contains(mesId);
    }
}
