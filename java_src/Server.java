import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Communication/Chat Server
 *
 *  java Server [port]
 *
 * This server listens on a given port (4004 by default), and assigns unique
 * names to connections.
 *
 * Protocol:
 *     First string client sends is interpreted as Nickname. Check uniqueness.
 *   Incoming commands:
 *    /c ID# :  Request to change the nickname
 *    /r DATA:  User sends some DATA
 *    Forward the rest;
 *
 *   Outgoing commands:
 *    /a ID:   Add user
 *    /d ID:   Delete user
 *    /c ID# : Command from the server to change the ID (if repeated)
 *    /r ID DATA: Tell users that user ID is sending DATA
 *
 *
 * History:
 *
 * 0.5 Add some data gathering
 * 0.4 Buggy raw transmission
 * 0.3 Numerical nicks
 * 0.2 Properly tells the other when somebody leaves. (2005/10/12)
 * 0.1 First chat version (de l'any de la picor)
 *
 * @author     David Gavilan
 * @created    2005/10/12
 */
public class Server {
       public final static String VERSION="0.5";
	static Vector llista = new Vector();
	static int port;


	/**
	 *  The main loop waits for connections, starting a new thread for every client.
	 *
	 * @param  args  You can pass the TCP port number.
	 */
	public static void main(String[] args) {
	       System.out.println("C/C server v"+VERSION);
		try {
			if (args.length == 0) {
				port = 4004;
			} else {
				port = Integer.parseInt(args[0]);
			}

			ServerSocket ss = new ServerSocket(port);
			
			System.out.println("Listening in port "+port+" ...");

			while (true) {
				Socket p = ss.accept();
				System.out.println("- Somebody trying to connect -");
				Entrada e = new Entrada(p);
				llista.addElement(e);
				e.start();
			}
		} catch (Exception ex) {
			System.out.println(ex);
		}
	}
}

/**
 *  Thread from every new connection.
 *
 * @author     David Gavilan
 * @created    2005/10/12
 */
class Entrada extends Thread {


	BufferedReader br;
	BufferedWriter bw;
	String nom;


	/**
	 *Constructor for the Entrada object
	 *
	 * @param  s                Description of the Parameter
	 * @exception  IOException  Description of the Exception
	 */
	Entrada(Socket s) throws IOException {
		System.out.println("- New thread -");
		InputStream is = s.getInputStream();
		br = new BufferedReader(new InputStreamReader(is));
		OutputStream os = s.getOutputStream();
		bw = new BufferedWriter(new OutputStreamWriter(os));
	}


	/**
	 *  Main processing method for the Entrada object
	 */
	public void run() {
		try {
			String str;
			System.out.println("- read user name -");
			str = br.readLine();
			nom = str;

			changeNick(str);

			sendUsers();
			addUser();

			while ((str = br.readLine()) != null) {
				String com;
				try {
					com = new String(str.substring(0, 2));
				} catch (Exception ex) {
					com = "";
				}
				// primer interpretem les comandes del client
				if (com.equals("/c")) {
					String old = nom;
					changeNick(str.substring(3));
					notifyChange(old);
				} else if (com.equals("/r")) { // this guy wanna send us sth raw...
					//can't find the bug with getting n ... so do it String based...
					//int n = Integer.parseInt(str.substring(3));
					//char[] data=new char[n];
					//br.read(data,0,n);
					// send the data to the others, but not me
					// DATA: /r 010101010101   a string like this
					Entrada tmp;
					for (Enumeration e = Server.llista.elements();
							e.hasMoreElements(); ) {
						tmp = (Entrada) (e.nextElement());
						if (tmp!=this) {
							tmp.bw.write("/r "+nom+" "+str.substring(3));
							tmp.bw.newLine();
							tmp.bw.flush();
							//tmp.bw.write(data,0,n);
							//tmp.bw.flush();
						} 
					}
				} else {
					// parlem amb la resta
					Entrada tmp;
					for (Enumeration e = Server.llista.elements();
							e.hasMoreElements(); ) {
						tmp = (Entrada) (e.nextElement());
						tmp.bw.write(nom+" "+str);
						tmp.bw.newLine();
						tmp.bw.flush();
					}
				}
			}
			delUser();
		} catch (Exception ex) {
			delUser();
		}

	}


	/**
	 *  Description of the Method
	 *
	 * @param  whom  Description of the Parameter
	 */
	void changeNick(String whom) {
		int num=0;
		boolean numerical=false;
		try { // check if it's numerical
			num = Integer.parseInt(whom);
			numerical = true;
		} catch (Exception ex) {
			numerical = false;
		}

		try {
			int nn = 1;
			String str = whom;
			while (nomRepetit(str)) {
				System.out.println("Repetit: " + str);
				if (numerical) {
					num++;
					str = "" + num;
				} else {
					str = whom + nn;
					nn++;
				}
			}
			if (nom != str) {
				// canviem el nom al client
				nom = str;
				bw.write("/c " + nom);
				bw.newLine();
				bw.flush();
			}
		} catch (Exception e) {
			System.out.println(e);
		}

	}


	/**
	 *  Description of the Method
	 *
	 * @exception  IOException  Description of the Exception
	 */
	public void sendUsers() throws IOException {
		Entrada tmp;
		for (int i = 0; i < Server.llista.size() - 1; i++) {
			tmp = (Entrada) Server.llista.elementAt(i);
			if (tmp!=this) {
				bw.write("/a " + tmp.nom);
				bw.newLine();
				bw.flush();
			}
		}
	}


	/**
	 *  Description of the Method
	 *
	 * @param  old  Description of the Parameter
	 */
	public void notifyChange(String old) {
		try {
			Entrada tmp;
			for (int i = 0; i < Server.llista.size(); i++) {
				tmp = (Entrada) Server.llista.elementAt(i);
				tmp.bw.write("/x " + old + " " + nom);
				tmp.bw.newLine();
				tmp.bw.flush();
			}
		} catch (IOException e) {
			System.out.println(e);
		}
	}


	/**
	 *  Adds a feature to the User attribute of the Entrada object
	 *
	 * @exception  IOException  Description of the Exception
	 */
	public void addUser() throws IOException {
		Entrada tmp;
		for (Enumeration e = Server.llista.elements();
				e.hasMoreElements(); ) {
			tmp = (Entrada) (e.nextElement());
			// don't send "me" to me!
			if (tmp!=this) {
				tmp.bw.write("/a " + nom);
				tmp.bw.newLine();
				tmp.bw.flush();
			}
		}
		System.out.println("Entra: " + nom);
	}


	/**
	 *  Description of the Method
	 */
	public void delUser() {
		try {
			Entrada tmp;
			for (Enumeration e = Server.llista.elements();
					e.hasMoreElements(); ) {
				tmp = (Entrada) (e.nextElement());
				if (tmp!=this) { 
					// petava per aqui!!! Quan algu es despenjava, a ell
					// mateix li volia enviar /d pero ja no existia!
					
					tmp.bw.write("/d " + nom);
					tmp.bw.newLine();
					tmp.bw.flush();
				}
			}
		} catch (Exception ex) {
			System.out.println(ex);
		}
		Server.llista.removeElement(this);
		System.out.println("Marxa: " + nom);

	}


	/**
	 *  This method checks for already existing user names in the list. 
	 *
	 * @param  who  Description of the Parameter
	 * @return      Description of the Return Value
	 */
	boolean nomRepetit(String who) {
		Entrada tmp;
		for (int i = 0; i < Server.llista.size(); i++) {
			tmp = (Entrada) Server.llista.elementAt(i);
			if (tmp != this && who.equals(tmp.nom)) {
				return true;
			}
		}
		return false;
	}

}
