import java.awt.*;
import java.awt.image.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;

/**
 *  This is the canvas for the client application.
 */
public class FireCanvas extends Canvas implements MouseMotionListener, Runnable {
	
	private volatile Thread animThread;
		
	private BufferedImage mBuffer;
	private BufferedImage background;
	private BufferedImage fireIm;
	protected Color bgColor = Color.black;
	protected Color fgColor = Color.white;
	protected Color fadeColor = new Color(0,0,0,2);

	int width;
	int height;
	String text = null;	
	Fire localFire;

	// network
	BufferedWriter bw;
	Vector fireList;
	Fire whoTalks;

	public FireCanvas(int width, int height) {
		super();
		setSize(width, height);
		this.width=width;
		this.height=height;
		bw=null;		

		setBackground(bgColor);
		setForeground(fgColor);
		
		localFire = new Fire();

		fireIm = new BufferedImage(Fire.WIDTH,Fire.HEIGHT,
			BufferedImage.TYPE_INT_ARGB);
    		
		fireList = new Vector();
		addMouseMotionListener(this);		
	}

	/**
	 * This method is important for the pack() call.
	 * super.getPreferredSize() returns negative dimensions and the window is
	 * not properly packed (just on Windows, on Mac OS X works fine - jdk1.4.2).
	 */
	public Dimension getPreferredSize() {
		Dimension size = super.getPreferredSize();
		//System.out.println(size);
		size.width = width;
		size.height = height;
		return size;
	}

	
	public void updateLocalName(int n) {
		localFire.setID(n);
		localFire.setColor();
	}

	public void setSize(int width, int height) {
		super.setSize(width, height);
		
		mBuffer = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
		background = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
		Graphics gi = mBuffer.getGraphics();
		gi.setColor(bgColor);
		gi.fillRect(0,0,width,height);
	}
	

	public void paint(Graphics g) {
		Graphics layer = mBuffer.getGraphics();
		drawOn(layer);
		g.drawImage(mBuffer,0,0,this);
	}
	
	public void update(Graphics g) {
		paint(g);
	}
	

	/** Draws some text in the background layer */
	public boolean placeText(String s) {
		if (text != null) return false; // I didn't paint last text yet!
		StringTokenizer st = new StringTokenizer(s);
		int who = Integer.parseInt(st.nextToken());
		localFire.setID(who);
		if (fireList.indexOf(localFire)<0) {
			whoTalks = localFire;
		} else {
			whoTalks = (Fire)fireList.elementAt(fireList.indexOf(localFire));
		}
		text = st.nextToken("\t");
		repaint();
		return true;
	}

	public void drawOn(Graphics g) {
		// clear everything
		g.setColor(bgColor);
		g.fillRect(0,0,width,height);

		// place text
		if (text != null) {
			Graphics gb = background.getGraphics();
			gb.setColor(fgColor);
			gb.drawString(text,
				whoTalks.getX()-(Fire.WIDTH>>1),
				whoTalks.getY()+10);
			text = null;
		} 
		g.drawImage(background,0,0,null);

		// fade a bit the background and update the background
		g.setColor(fadeColor);
		g.fillRect(0,0,width,height);
		Graphics bg = background.getGraphics();
		bg.drawImage(mBuffer,0,0,this);
		
		// local fire object
		fireIm.setRGB(0,0,Fire.WIDTH,Fire.HEIGHT,localFire.getLine(),0,Fire.WIDTH);
		int hw = localFire.getX() - (Fire.WIDTH>>1);
		int hh = localFire.getY() - Fire.HEIGHT;
		g.drawImage(fireIm,hw, hh,null);
		
		// remote fires
		for (Enumeration e = fireList.elements(); e.hasMoreElements(); ) {
			Fire ff = (Fire) (e.nextElement());
			// without command below, it just mirrors the same object, for testing ;)
			fireIm.setRGB(0,0,Fire.WIDTH,Fire.HEIGHT,ff.getLine(),0,Fire.WIDTH);
			hw = ff.getX() - (Fire.WIDTH>>1);
			hh = ff.getY() - Fire.HEIGHT;
			g.drawImage(fireIm,hw, hh,null);			
		}
		
	}

  
	public void mouseMoved(MouseEvent e) {
		localFire.setPosition(e.getPoint());
		repaint();
	}

	public void mouseDragged(MouseEvent e) {
	}


	// Thread
	// ---------------------------------------------------------------------------------
	public void start() {
		animThread = new Thread(this);
		animThread.start();
	}
	
	public void stop() {
		animThread = null;
	}
	
	public void run() {
		Thread me = Thread.currentThread();
		while (animThread == me) {
			localFire.genLine();
			// remote fires
			for (Enumeration e = fireList.elements(); e.hasMoreElements(); ) {
				Fire ff = (Fire) (e.nextElement());
				ff.update();
			}			
			repaint();

			try {
				send();
				Thread.currentThread().sleep(50);
			} catch (InterruptedException e) {
			} catch (IOException e) {
				System.err.println("send: "+e);
			}
		}
	}


	// Network
    // ---------------------------------------------------------------
	private void send() throws IOException {
		if (bw!=null) {
			// tell the server to start raw mode and the number of chars to receive
			//bw.write("/r "+localFire.getLength());
			//bw.newLine();
			//bw.flush();
			int x=localFire.getX();
			int y=localFire.getY();
			bw.write("/r "+x+" "+y+" "+localFire.getLineString());
			bw.newLine();
			bw.flush();
		}
	}

	public void setServer(BufferedWriter bw) {
		this.bw = bw;
	}
	
	public void addFire(String name) {
		System.out.println("adding... "+name);
		Fire ff = new Fire();
		ff.setID(name);
		ff.setColor();
		fireList.add(ff);
		System.out.println("added "+ff.getID());
	}
	
	public void updateRemote(String unparsed) {
		StringTokenizer st = new StringTokenizer(unparsed);
		String who = st.nextToken();
		String posX = st.nextToken();
		String posY = st.nextToken();
		String line = null;
		if (st.hasMoreTokens()) {
		   line = st.nextToken();
		   // check if we have enough data
		   if (line.length() < Fire.WIDTH) line = null;
		} // otherwise, we ll just generate a random line
		
		try {
		localFire.setID(who);
		//System.out.println("searching; "+who+", "+fireList.indexOf(localFire));
		Fire ff = (Fire)fireList.elementAt(fireList.indexOf(localFire));
		//System.out.println("who: "+ff.getID());
		ff.setPosition(posX, posY);
		if (line!=null) {
		   ff.setLine(line);
		} else {
		   ff.genLine();
		}
		} catch (Exception exc) {
		  System.err.println("ID "+who+" is not (yet) in my list.");
		}
		
	}
	
	public void delFire(String name) {
		localFire.setID(name);
		//System.out.println("searching; "+who+", "+fireList.indexOf(localFire));
		fireList.remove(fireList.indexOf(localFire));	
	}
		
}