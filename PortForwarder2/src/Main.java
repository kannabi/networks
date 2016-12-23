

public class Main {

    public static void main(String[] args) {
        PortForwarder portForwarder = new PortForwarder(Integer.parseInt(args[0]), args[1], Integer.parseInt(args[2]));
    }
}
