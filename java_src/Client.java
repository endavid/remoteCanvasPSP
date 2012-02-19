import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Fire Client
 *
 * java Client [host [port]]
 *
 * Version History:
 * 0.6:  Solved window size problem under Windows, changed colors, minor bugs.
 * 0.5:  Command line arguments
 * 0.4:  Most things work. Sometimes when connecting, Client exception (-1)
 * 0.3:  Changed to Strings, but fire not updated?? position ok
 * 0.2:  Buggy raw transmission.
 * 0.1:  FireCanvas + Simple chat capabilities
 */
public class Client extends JFrame implements Runnable, ActionListener {
	public static final String APP_NAME = "Fire Client";
	public static final String VERSION = "0.6";
	public static final int CANVAS_WIDTH = 480;
	public static final int CANVAS_HEIGHT=272;

	FireCanvas fcanvas;
	JTextField textField;
	String host="localhost";
  	int port=4004;
	BufferedWriter bw;
	Thread chat;
	String nom;

  class Accions extends KeyAdapter {
	public void keyTyped(KeyEvent e) {
	    try {
		if (e.getKeyChar()==KeyEvent.VK_ENTER) {
		    bw.write(textField.getText()+"\n");
		    bw.flush();
		    //textArea.append(textField.getText()+"\n");
		    textField.setText("");
		}
	    } catch(Exception ex) {
			System.err.println("Accions:"+ex);
	    }
	}
  } // end class Accions
	
	public Client() {
		fcanvas = new FireCanvas(CANVAS_WIDTH, CANVAS_HEIGHT);
		JPanel toolBar = new JPanel();
		JButton cButton = new JButton("connect");
		cButton.setActionCommand("connect");
		cButton.addActionListener(this);
	    textField = new JTextField(20);
	    textField.addKeyListener(new Accions());

		toolBar.add(cButton);
		toolBar.add(textField);
	

		Container contentPane = getContentPane();
		contentPane.setLayout(new BorderLayout());
		contentPane.add(fcanvas,BorderLayout.CENTER);
		contentPane.add(toolBar,BorderLayout.SOUTH);

		fcanvas.start(); // start thread
		
		chat = null;
		addWindowListener(
			new WindowAdapter() {
				public void windowClosing(WindowEvent e) {
					System.exit(0);
				}
			});
	}

    public void actionPerformed(java.awt.event.ActionEvent e) {
		String s = e.getActionCommand();
		if (s.equals("connect")) { // connects to the server
			if (chat == null) {
				chat = new Thread(this);
				chat.start();
			}
		}
    }
  public void run() {

	try {
		System.out.println("Connecting to "+host+"... port "+port);
	    Socket s = new Socket(host, port);

	    BufferedReader br = new BufferedReader ( new InputStreamReader ( s.getInputStream() ));

	    bw = new BufferedWriter(new OutputStreamWriter(s.getOutputStream()));

	    //Send number name. The server will return an updated number
		nom="1";
	    bw.write(nom);
		bw.newLine();
	    bw.flush();

		fcanvas.setServer(bw);

	    String str;
	    int i=1;
	    while((str=br.readLine())!=null) {
		//System.out.println("rcvd: "+str);
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
			fcanvas.updateLocalName(Integer.parseInt(nom));
		} else if (com.equals("/x")) {
		    System.out.println(str.substring(0,2)+":"+str.substring(3));			
		    StringTokenizer st=new StringTokenizer(str.substring(3)," ");
		    delUser(st.nextToken());
		    addUser(st.nextToken());
		} else if (com.equals("/r")) { // this guy wanna send us sth raw...
			//int n = Integer.parseInt(str.substring(3));
			//char[] data=new char[n];
			//int leidos = br.read(data,0,n);
			fcanvas.updateRemote(str.substring(3));
			//System.out.println(str+" read");
		} else if (!str.equals("")){
		    System.out.println(str);			
			while (!fcanvas.placeText(str)) {
				Thread.sleep(10);}; // wait till u can place the text
		}
	    }
	} catch( Exception e) {
	    System.err.println("Client: "+e);
		chat = null;
	}

    }

	void addUser(String nom) {
		fcanvas.addFire(nom);
	}

	void delUser(String nom) {
		fcanvas.delFire(nom);
	}



	
	public static void main(String args[]) {			
		Client window = new Client();
		if (args.length>0) {
			window.host = args[0];
		}
		if (args.length>1) {
			window.port = Integer.parseInt(args[1]);
		}
		
		window.setTitle(APP_NAME+ " v."+VERSION);
		window.pack();
		window.setVisible(true);
	}
}