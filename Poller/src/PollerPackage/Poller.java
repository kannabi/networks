package PollerPackage;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;

/**
 * Created by kannabi on 05.01.17.
 */
public class Poller {

    public static void executePost() {
        try {
            URL yahoo = new URL("http://www.google.com/");
            URLConnection yc = yahoo.openConnection();
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(
                            yc.getInputStream()));
            String inputLine;

            while ((inputLine = in.readLine()) != null)
                System.out.println(inputLine);
            in.close();
        } catch (IOException e){
            e.printStackTrace();
        }
    }
}
