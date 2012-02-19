import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Robot Client (Listener/Output)
 * MouseClient generates the moves.
 *
 * This client just listen to remote movements of the mouse.
 *
 * Version History:
 * 0.2:  Moves smoothly
 * 0.1:  Fire Client 0.4
 */
public class RobotClient implements Runnable {
	public static final String APP_NAME = "Robot Client";
	public static final String VERSION = "0.2";
	public static final int CANVAS_WIDTH = 480;
	public static final int CANVAS_HEIGHT = 272;
	
	String host;
  	int port;
	BufferedWriter bw;
	Thread chat;
	String nom;
	Vector clientList = new Vector(20,2);
	Robot robot;
	double xRatio;
	double yRatio;
	Point position;
	
	public RobotClient(String host, int port) {
		this.host = host;
		this.port = port;
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		xRatio = (double)screenSize.width/(double)CANVAS_WIDTH;
		yRatio = (double)screenSize.height/(double)CANVAS_HEIGHT;
		position = new Point(0,0);
		
		chat = new Thread(this);
		chat.start();		
	}

  public void run() {

	try {
		robot = new Robot();
	    Socket s = new Socket(host, port);

	    BufferedReader br = new BufferedReader ( new InputStreamReader ( s.getInputStream() ));

	    bw = new BufferedWriter(new OutputStreamWriter(s.getOutputStream()));

	    //Send number name. The server will return an updated number
		nom="1";
	    bw.write(nom);
		bw.newLine();
	    bw.flush();


	    String str;
	    int i=1;
		int previousKey = 0;
		
	    while((str=br.readLine())!=null) {
		String com;
		try {
	             com = new String(str.substring(0,2));
		} catch (Exception ex) {
		    com = "";
		}
		// primer interpretem les comandes del servidor
		if (com.equals("/a")) {
			System.out.println(str.substring(0,2)+":"+str.substring(3));
		    addUser(str.substring(3));
		} else if (com.equals("/d")) {
		    System.out.println(str.substring(0,2)+":"+str.substring(3));			
		    delUser(str.substring(3));
		} else if (com.equals("/c")) {
		    nom = str.substring(3);
		    System.out.println(str.substring(0,2)+":"+str.substring(3));
		} else if (com.equals("/x")) {
		    System.out.println(str.substring(0,2)+":"+str.substring(3));			
		    StringTokenizer st=new StringTokenizer(str.substring(3)," ");
		    delUser(st.nextToken());
		    addUser(st.nextToken());
		} else if (com.equals("/r")) { // this guy wanna send us sth raw...
			StringTokenizer st = new StringTokenizer(str.substring(3));
			String who = st.nextToken();
			double posX = (double)Integer.parseInt(st.nextToken());
			double posY = (double)Integer.parseInt(st.nextToken());
			int key = 0;
			if (st.hasMoreTokens()) { // key pressed
				try {
					key = Integer.parseInt(st.nextToken());
				} catch (Exception exc) {
					key = 0;
				}
			}
			
			
			double destX = (xRatio*posX), destY = (yRatio*posY);
			double x = position.getX(), y = position.getY();
			double length = Math.sqrt((destX-x)*(destX-x)+(destY-y)*(destY-y));
			//System.out.println("from: "+x+", "+y+" to "+destX+", "+destY);
			int xx=(int)destX,yy=(int)destY;
			  // if xx=0,yy=0, restart from zero!!!!
			// do a loop for big loops
			//if (length > 8) {
			//for (int k = 1; k<(int)length;k+=4) {
			//	xx = (int)(x+(double)k*(destX-x)/length);
			//	yy = (int)(y+(double)k*(destY-y)/length);
			//	robot.mouseMove(xx,yy);
				//Thread.sleep(1); // it introduces LAG problem!
			//}}
			xx=(int)destX;yy=(int)destY;
			position.setLocation(xx,yy);
			robot.mouseMove(xx,yy);
			if (key != previousKey) { // ignore continous pressings
			switch (key) {
				case 1:
					robot.mousePress(InputEvent.BUTTON1_MASK);
					break;
				case 2:
					robot.mousePress(InputEvent.BUTTON3_MASK);
					break;
				default:
					robot.mouseRelease(InputEvent.BUTTON1_MASK);
					robot.mouseRelease(InputEvent.BUTTON3_MASK);
			}
			}
			previousKey = key;		

		} else if (!str.equals("")){
		    System.out.println(str);			
		}
	    }
	} catch( Exception e) {
	    System.err.println("Client: "+e);
		chat = null;
		clientList.removeAllElements();
	}

    }


    public void addUser(String nombre) {
		clientList.addElement(nombre);
    }

    public void delUser(String nombre) {
		clientList.removeElement(nombre);
    }

	

	
	public static void main(String args[]) {
		String host = "localhost";
		int port = 4004;
		Client window = new Client();
		if (args.length>0) {
			host = args[0];
		}
		if (args.length>1) {
			port = Integer.parseInt(args[1]);
		}
		RobotClient task = new RobotClient(host, port);
		
		System.out.println(APP_NAME+ " v"+VERSION);
	}
}