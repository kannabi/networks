package NodePackage;

import java.io.IOException;
import java.net.*;
import java.util.*;
import java.util.logging.Logger;

import ConstPackage.Const;

/**
 * Всего в программе четыре нити.
 * Первая, она же изначальная, читает вводимые сообщения и кладет их в очередь для отправки; создает и запускает все другие нити
 * Listener слушает приходящие соообщения и обрабаотывает их
 * Sender берет из очереди сообщений для отправки новые сообщения и отправляет их
 * Resender раз в несколько секунд проверяет, есть ли сообщения, доставка которых подтверждена не со всех адресов.
 * При необходимости отправляет их снова.
 *
 * Есть четыре типа сообщений.
 * Первый тип предназначен для передачи текстовых сообщений; его структура:
 * <uuid><messageType><messageUuid><text>
 * В данном случе messageType == Const.MES
 * Второй тип предназначен для передачи подтверждения того, что сообщение дошло.
 * <uuid><Const.CONF><messageUuid>
 * (Третья составляющая -- идентификатор сообщения, которое было успешно доставлено)
 * Третий тип сообщения рассылается отключающимся узлом для информирования других участниув.
 * <uuid><Const.SER><адрес, куда следует переподключиться><порт>
 * Это сообщение отправляется всему списку контактов. Если узел получил это сообщение, и там указаны его данные, он
 * игнорирует это сообщение
 * Четвертый тип предназначен для уведомления отключающегося узла, что остальные услышали его.
 * <uuid><Const.BYE><uuid>
 * Первый уникальный идентификатор -- откуда пришло подтверждение. Во второй записан уникальный иентификатор отключающегося узла
 */

public class Node extends Thread {

    private static Logger log = Logger.getLogger(Node.class.getName());

    private boolean grandpa;

    private int selfPort;
    private int percentOfLost;
    private String ancIP;
    private int ancPort;
    private String ancUuid;

    private volatile DatagramSocket listeningSocket;
    //набор сообщений, который были отправлены, но чья доставка еще не подтверждена
    private volatile MessageQueueConf unconfMessage = new MessageQueueConf();
    //набор сообщений, которые необходимо отправить
    private volatile MessageQueue messageQueue = new MessageQueue();
    //кеш сообщений, в который записываются уникальные идентификаторы всех пришедших сообщений.
    InputMessageCash inputMessageCash = new InputMessageCash();

    //Таблица с соединениями: как предок, так и потомки
    private volatile Hashtable <String, Connection> connectionMap = new Hashtable<>();
    private final int DATAGRAMM_SIZE = Const.DATAGRAMM_SIZE;

    private UUID uuid = UUID.randomUUID();

    //Контсруктор для листа
    public Node (String percentOfLost, String selfPort, String ancIP, String ancPort){
        log.info("Start leaf");

        this.selfPort = Integer.parseInt(selfPort);
        this.percentOfLost = Integer.parseInt(percentOfLost);
//        this.percentOfLost = Const.PERCENT;
        this.grandpa = false;
        this.ancPort = Integer.parseInt(ancPort);
        this.ancIP = ancIP;
        initSocket();

        try{
            connectParent(ancIP, Integer.parseInt(ancPort));
            working();
        } catch (ConnectionException e){
            e.printStackTrace();
        }
    }

    //Конструткор для корня
    public Node (String percentOfLost, String selfPort){
        log.info("Start root");
        grandpa = true;
        this.percentOfLost = Integer.parseInt(percentOfLost);
//        this.percentOfLost = Const.PERCENT;
        this.selfPort = Integer.parseInt(selfPort);
        this.ancUuid = uuid.toString();

        initSocket();
        working();
    }

    private void sayGoodBye(){
        Resender resender;
        System.out.println("Shutdown hook ran!");
        String message;
        if (grandpa) {
            Set<String> uuids = connectionMap.keySet();
            Iterator<String> iterator = uuids.iterator();
            Connection futureRoot = connectionMap.get(iterator.next());
            message = uuid.toString() + Const.SER + Const.SEP + futureRoot.getAddr() + Const.SEP + Integer.toString(futureRoot.getPort());
            messageQueue.put(new Message(message, uuids));
        } else {
            message = uuid.toString() + Const.SER + Const.SEP + ancIP + Const.SEP + Integer.toString(ancPort);
            messageQueue.put(new Message(message, connectionMap.keySet()));
        }

        unconfMessage.addMessage(uuid.toString(), copySet(connectionMap.keySet()), message);

        try {
            Thread.sleep(3000);
        } catch (InterruptedException e){
            e.printStackTrace();
        }
        listeningSocket.close();
    }

    //Основная функция нити, которая читает сообщения из терминала и кладет в очерель для отправки.
    private void working(){
        Thread listeningThread = new Thread(new Listener());
        Thread sender = new Thread(new Sender());
        Thread resender = new Thread(new Resender());
        listeningThread.start();
        sender.start();
        resender.start();

        Scanner in = new Scanner(System.in);
        String text;
        String message;
        String ID;
        Message mesPack;

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            sayGoodBye();
        }));

        while (true) {
            text = in.nextLine();
            ID = UUID.randomUUID().toString();
            message = uuid.toString() + Const.MES + ID + text;
            mesPack = new Message(message, connectionMap.keySet());
            messageQueue.put(mesPack);
            unconfMessage.addMessage(ID, copySet(connectionMap.keySet()), message);
        }
    }

    private void initSocket(){
        try {
            listeningSocket = new DatagramSocket(selfPort);
            listeningSocket.setSoTimeout(Const.WAIT_TIME);
        } catch (SocketException e) {
//            System.err.println("Listener thread");
            e.printStackTrace();
        }
    }

    //Следующие пять функций являются вспомогательными для обрадотки пакетов и сообщений
    private String getMessageFromPacket (DatagramPacket packet){
        return new String(packet.getData(), 0 ,packet.getLength());
    }

    private String getUuid (String message){
        return message.substring(0, Const.ID_LENGTH);
    }

    private String getMessageBody (String message){
        return message.substring(Const.ID_LENGTH*2 + Const.TYPE_LENGTH, message.length());
    }

    private String getMessageType (String message){
        return message.substring(Const.ID_LENGTH, Const.ID_LENGTH + Const.TYPE_LENGTH);
    }

    private String getMessageUuid (String message){
        return message.substring(Const.ID_LENGTH + Const.TYPE_LENGTH, Const.ID_LENGTH * 2 + Const.TYPE_LENGTH);
    }

    //Осуществляется пять попыток подключиться к узлу
    //Если узел так и не ответил, программа завершается
    private void connectParent(String ancIP, int ancPort) throws ConnectionException {
        try{
            log.info("Try to connect to parent");
            int i;
            String strUuid = uuid.toString();
            System.out.println("My id: " + strUuid);

            byte[] buf = strUuid.getBytes();
            byte[] recBuf = new byte[DATAGRAMM_SIZE];

//            InetAddress address = InetAddress.getByName(ancIP);
            InetAddress address = InetAddress.getLocalHost();
            DatagramPacket packet = new DatagramPacket(buf, buf.length,
                    address, ancPort);
            DatagramPacket recPacket = new DatagramPacket(recBuf, recBuf.length);

            System.out.println("sending packet: " + packet + " buf: " + buf.toString());
            for (i = 0; i < Const.MAX_NUM_OF_TRY; ++i){
                try{
                    listeningSocket.send(packet);
                    log.info("Hello packet has been sent");
                    listeningSocket.receive(recPacket);
                    break;
                } catch (SocketTimeoutException e){
                    continue;
                }
            }

            if (i == Const.MAX_NUM_OF_TRY)
                throw new ConnectionException("Cannot access parent");

            System.out.println("packet = " + getMessageFromPacket(recPacket));
            ancUuid = getMessageFromPacket(recPacket);

            connectionMap.put(getMessageFromPacket(recPacket), new Connection(packet.getAddress(), packet.getPort()));
            System.out.println("Connection to parent is successful");
        }  catch (SocketException e){
            throw new ConnectionException("Can't open socket");
        } catch(UnknownHostException e){
            throw new ConnectionException("Can't resolve IP");
        } catch (IOException e){
            throw new ConnectionException("Can't send datagramm");
        }
    }


    private class Listener implements Runnable {

        private boolean exit;

        private void confirmMessageRecv(DatagramPacket packet){
            String message = getMessageFromPacket(packet);
            unconfMessage.confirm(getMessageUuid(message), getUuid(message));
        }

        private void resend(DatagramPacket packet){
            try {
                Message ansMessage;
                String message = getMessageFromPacket(packet);

                if(inputMessageCash.contain(getMessageUuid(message)))
                    return;
                else
                    inputMessageCash.add(getMessageUuid(message));
                System.out.println(getUuid(message) + ": " + getMessageBody(message));

                String messageUuid = UUID.randomUUID().toString();
                String ans = uuid.toString() + Const.MES + messageUuid + getMessageBody(message);
                String confAns = uuid.toString() + Const.CONF + getMessageUuid(message);
                byte[] buf = confAns.getBytes();

                DatagramPacket confPack = new DatagramPacket(buf, buf.length, packet.getAddress(), packet.getPort());
                listeningSocket.send(confPack);

                Set<String> addr = copySet(connectionMap.keySet());
                addr.remove(getUuid(message));
                ansMessage = new Message(ans, addr);
                messageQueue.put(ansMessage);
                unconfMessage.addMessage(messageUuid, addr, ans);
            } catch (IOException e){
                e.printStackTrace();
            }
        }

        private void workWithServiceMessage(DatagramPacket packet){
            try {
                String message = getMessageFromPacket(packet);
                String fromUuid = getUuid(message);

                if(inputMessageCash.contain(fromUuid))
                    return;
                else
                    inputMessageCash.add(fromUuid);

                byte[] buf = (uuid.toString() + Const.BYE + fromUuid).getBytes();
                DatagramPacket pack = new DatagramPacket(buf, buf.length, packet.getAddress(), packet.getPort());
                if(!connectionMap.containsKey(fromUuid)) {
                    listeningSocket.close();
                } else {
                    if (getUuid(message).equals(ancUuid)) {
                        String[] parsedMessage = message.split(Const.SEP);
                        if (parsedMessage[2].equals(Integer.toString(selfPort))) { // && parsedMessage[1].equals(InetAddress.getLocalHost().toString())) {
                            grandpa = true;
                        } else {
                            ancIP = parsedMessage[1];
                            ancPort = Integer.parseInt(parsedMessage[2]);
                            connectParent(ancIP, ancPort);
                        }
                    }
                    connectionMap.remove(fromUuid);
                }
            } catch (ConnectionException e) {
                e.printStackTrace();
            }
        }

        private void exiting(){
            DatagramPacket packet = new DatagramPacket(new byte[DATAGRAMM_SIZE], DATAGRAMM_SIZE);
            String message;

            while(!unconfMessage.isEmpty()){
                try {
                    listeningSocket.receive(packet);
                    message = getMessageFromPacket(packet);
                    if(getMessageType(message).equals(Const.BYE)){
                        unconfMessage.confirm(uuid.toString(), getUuid(message));
                    }
                } catch (IOException e){
                    e.printStackTrace();
                }
            }

            exit = false;
        }

        private void answerToNewDesc (DatagramPacket pack){
            try {
                String message = getMessageFromPacket(pack);
                System.out.println("New user " + message + ' ' + pack.getAddress().getHostAddress() + ' ' + (pack.getPort()));

                Connection connection = new Connection(pack.getAddress(), pack.getPort());
                connectionMap.put(getUuid(message), connection);

                String ans = uuid.toString();
                byte[] ansBuf = ans.getBytes();
                DatagramPacket ansPack = new DatagramPacket(ansBuf, ansBuf.length, connection.getAddr(), connection.getPort());

                listeningSocket.send(ansPack);
            } catch (IOException e){
                e.printStackTrace();
            }
        }

        private void processPacket (DatagramPacket packet){
            String message = getMessageFromPacket(packet);

            if (!connectionMap.containsKey(getUuid(message)) && message.length() == Const.ID_LENGTH) {
                answerToNewDesc(packet);
                return;
            }

            switch (getMessageType(message)){
                case Const.MES: {
                    resend(packet);
                    break;
                }
                case Const.SER:{
                    workWithServiceMessage(packet);
                    break;
                }
                case Const.CONF:{
                    confirmMessageRecv(packet);
                    break;
                }
                case Const.BYE:{
                    exiting();
                    break;
                }
            }
        }

        @Override
        public void run(){
            log.info("Listener start");
            byte[] buf = new byte[DATAGRAMM_SIZE];
            DatagramPacket recPacket = new DatagramPacket(buf, buf.length);
            Random random = new Random(System.currentTimeMillis());
            exit = true;

            try {
                while (exit) {
                    try {
                        listeningSocket.receive(recPacket);
                        if (random.nextInt(100) > percentOfLost)
                            processPacket(recPacket);
                    } catch (SocketTimeoutException e) {
    //                        log.info("socket timeout");
                    }
                }
            }  catch (IOException e){
                e.printStackTrace();
            }
        }
    }

    private class Sender implements Runnable{
        Message message;


        @Override
        public void run(){
            while (messageQueue.isEmpty()){
                message = messageQueue.get();
                sendToContacts(message.getAddr(), message.getBody());
            }
        }

        private void sendToContacts (Set <String> uuids, String message){
            try {
                DatagramPacket resPack;
                byte[] buf = message.getBytes();

                for (String str : uuids){
                    resPack = new DatagramPacket(buf, buf.length, connectionMap.get(str).getAddr(), connectionMap.get(str).getPort());
                    listeningSocket.send(resPack);
                }
            } catch (IOException e){
                e.printStackTrace();
            }
        }
    }

    private class Resender implements Runnable{

        private void resend(){
            Set<String> messageIDs;
            Message resMessge;

            if(unconfMessage.isEmpty())
                return;

            messageIDs = unconfMessage.getMessagesSet();

            for (String it : messageIDs){
                resMessge = new Message(unconfMessage.getMessage(it), unconfMessage.getUnconfirmedAddresses(it));
                messageQueue.put(resMessge);
            }
        }

        @Override
        public void run(){
            try {
                while (true){
                    Thread.sleep(5000);
                    if(!unconfMessage.isEmpty())
                        resend();
//                    Thread.sleep(2000);
                }
            } catch (InterruptedException e){
                e.printStackTrace();
            }
        }
    }

    private Set<String> copySet (Set<String> from){
        Set<String> to = new TreeSet<>();
        for (String aFrom : from)
            to.add(aFrom);

        return to;
    }

    private void printMap(){
        Set<String> keys = connectionMap.keySet();
        Iterator<String> iterator = keys.iterator();
        String str;
        System.out.println("print map");

        while (iterator.hasNext()) {
            str = iterator.next();
            System.out.println(str+ ' ' + connectionMap.get(str).getAddr().getHostAddress() + ' ' + connectionMap.get(str).getPort());
        }
    }
}