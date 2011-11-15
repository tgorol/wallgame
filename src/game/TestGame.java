package wg.games;

import java.util.Iterator;
import java.util.List;
import java.util.Collections;
import java.util.ArrayList;
import java.util.Random;

import javax.swing.SwingUtilities;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.BorderFactory;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Point;
import java.awt.Graphics2D;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.BufferedInputStream;
import java.net.Socket;

import org.newsclub.net.unix.AFUNIXServerSocket;
import org.newsclub.net.unix.AFUNIXSocketAddress;

class Hit extends Point{
    protected Color color;

    public Hit(){
        this(0, 0);
    }

    public Hit(int x, int y){
        this(new Point(x,y));

    }

    public Hit(Point p){
        this(p, Color.BLUE);
    }

    public Hit(int x, int y, Color color){
        this(new Point(x, y), color);
    }

    public Hit(Point p, Color color){
        super(p);
        this.color = color;
    }

    public Color getColor(){
        return this.color;
    }

}
class GamePanel extends JPanel{
    List<Hit> points = null;

    public GamePanel(List<Hit> points){
        setBorder(BorderFactory.createLineBorder(Color.black));
        this.points = points;
        setBackground(Color.BLACK);
    }

    public Dimension getPreferredSize(){
        return new Dimension(500,500);
    }

    public void paintComponent(Graphics g){
        super.paintComponent(g);

        Dimension size = getSize();

        synchronized(points){
            Iterator<Hit> i = points.iterator();
            while (i.hasNext()){
                Hit hit = i.next();
                g.setColor(hit.getColor());
                //System.out.println(hit);
                g.fillOval(
                        (int)(hit.getX()/100.0 * size.width), 
                        (int)(hit.getY()/100.0 * size.height),
                        10, 10); 
            }
        }
    }
}

/**
 *   A simple demo server
 *  
 *  @author Tomasz Gorol
 *  @see TestGame
 */
public class TestGame implements Runnable {
    List<Hit> hits;
    HitReader hitReader;
    JPanel display;

    public static void main(String[] args) throws IOException{
        final String[] arguments = args;
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                createAndShowGUI(arguments); 
            }
        });
    }

    private static void createAndShowGUI(String args[]){
        TestGame tg = new TestGame(args);

    }

    public TestGame(String[] args) {
        try{
            System.out.printf("path = " + args[0]);
            hitReader = new HitReader(args[0]);
        }catch (IOException e){
        }            

        System.out.println("Created GUI on EDT? "+
                SwingUtilities.isEventDispatchThread());
        JFrame f = new JFrame("Swing Paint Demo");
        f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        hits = Collections.synchronizedList(new ArrayList<Hit>());
        display = new GamePanel(hits);
        f.add(display);
        f.pack();
        f.setVisible(true);

        new Thread(this).start();
    }

    public void run(){
        byte[] buffer = new byte[1024];
        int readed;
        int index;
        int length;

        try{
            while (!Thread.interrupted()){
                String line;
                Socket s = hitReader.listen();
                BufferedInputStream is = new BufferedInputStream(s.getInputStream());
                readed = is.read(buffer, 0, buffer.length);
                line = new String(buffer,0,readed, "UTF-8");

                String[] list = line.split("\\s+");
                length = (list.length / 2) * 2;

                for (index = 0; index < length; index += 2){
                    int x, y;
                    x = Integer.valueOf(list[index]);
                    //System.out.println("X = " + x);
                    y = Integer.valueOf(list[index + 1]);
                    //System.out.println("Y = " + y);
                    hits.add(new Hit(x, y, ColorFactory.getRandom()));
                }
                display.repaint();

                s.close();
            }
        }catch (IOException e){

        }

    }
}

class HitReader {

    final File socketFile;
    AFUNIXServerSocket server;

    public HitReader(String pathName) throws IOException{
        socketFile = new File(pathName);
        server = AFUNIXServerSocket.newInstance();
        server.bind(new AFUNIXSocketAddress(socketFile));
    }

    public Socket listen() throws IOException{
        //System.out.println("Waiting for connection...");
        Socket sock = server.accept();

        return sock;
    }
}

class ColorFactory {
    static Random random = null;

    static Color getRandom(){
        if (random == null){
            random = new Random();
        }

        return new Color(
                random.nextInt(255),
                random.nextInt(255),
                random.nextInt(255),
                255);
    }

    static Color getRandom(int alfa){
        if (random == null){
            random = new Random();
        }

        return new Color(
                random.nextInt(255),
                random.nextInt(255),
                random.nextInt(255),
                alfa);
    }



}
