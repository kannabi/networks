package MainPackage;

import NodePackage.Node;

public class Main {
    public static void main(String[] args) {
        int self_port;
        Node node;

        if (args.length == 2){
            node = new Node(args[0], args[1]);
        } else if (args.length == 4){
            node = new Node(args[0], args[1], args[2], args[3]);
        } else {
            System.out.println("Wrong parameters number");
        }
    }
}
