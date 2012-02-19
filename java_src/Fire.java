import java.awt.*;
import java.util.Random;

/** The fire pointers */
public class Fire {
	public static final int WIDTH=60;
	public static final int HEIGHT=80;


	int id; // this ID should be unique;
	Point position;
	
	public int getX() {
		return (int)position.getX();
	}
	
	public int getY() {
		return (int)position.getY();
	}
	
	public void setPosition(Point p) {
		position.setLocation(p);
	}
	
	public void setPosition(String x, String y) {
		position.setLocation(Integer.parseInt(x), Integer.parseInt(y));
	}
	
	public void setID(String num) {
		setID(Integer.parseInt(num));
	}
	
	public void setID(int num) {
		id = num;
	}
	
	public void setColor() {
		setColor(id);
	}
	
	
	public int getID() {
		return id;
	}
	
  private int[] firePal=new int[256];
	int[] linea;
	int[] dline;

	public Fire() {
		this(1);
	}
	
	public Fire(int id) {
		linea=new int[WIDTH*HEIGHT];
		dline=new int[WIDTH*HEIGHT];
		this.id = id;
		position = new Point(0,0);
		setColor();
	}
	
	
  public void setColor(int n) {
	switch(n) {
		case 1:
			iniRedFire();
			break;
		case 2:
			iniGreenFire();
			break;
		case 3:
		        iniCyanFire();
		        break;
		case 4:
			iniBlackFire();
			break;
		default:
			iniRedFire();
	}
  }

  private void iniRedFire() {
    int j;
    for (int i=0;i<=40;i++) { // blue halo
    	firePal[i]=128*i/40;
    }
    for (int i=40;i<80;i++) {
    	j=128*(i-40)/40;
    	firePal[i]=j<<16|0|128-j;
    }
    for (int i=80;i<120;i++) {
    	j=128*(i-80)/40;
    	firePal[i]=(128+j)<<16|0;
    }
    for (int i=120;i<160;i++) {
    	j=255*(i-120)/40;
    	firePal[i]=255<<16|j<<8;
    }
    for (int i=160;i<256;i++) {
    	j=255*(i-160)/(256-160);
    	firePal[i]=255<<16|255<<8|j;
    }
    double d;
    for (int i=0;i<256;i++) { //alpha
        d=32.0*Math.log(i+1)/Math.log(2.0);
        j=(int)d; j=(j>255)?255:j;        
        firePal[i]|=j<<24;
    }         
  }

  private void iniGreenFire() {
    int j;
    for (int i=0;i<=40;i++) {
    	firePal[i]=128*i/40;
    }
    for (int i=40;i<80;i++) {
    	j=128*(i-40)/40;
    	firePal[i]=j<<8|128-j;
    }
    for (int i=80;i<120;i++) {
    	j=128*(i-80)/40;
    	firePal[i]=(128+j)<<8|0;
    }
    for (int i=120;i<160;i++) {
    	j=255*(i-120)/40;
    	firePal[i]=j<<16|255<<8;
    }
    for (int i=160;i<256;i++) {
    	j=255*(i-160)/(256-160);
    	firePal[i]=255<<16|255<<8|j;
    }
    double d;
    for (int i=0;i<256;i++) { //alpha
        d=32.0*Math.log(i+1)/Math.log(2.0);
        j=(int)d; j=(j>255)?255:j;        
        firePal[i]|=j<<24;
    }         
  }

  private void iniCyanFire() {
    int j;
    for (int i=0;i<=80;i++) { // blue halo
    	firePal[i]=128*i/80;
    }
    for (int i=80;i<120;i++) {
    	j=128*(i-80)/40;
    	firePal[i]=j<<8|128;
    }
    for (int i=120;i<160;i++) {
    	j=255*(i-120)/40;
    	firePal[i]=j<<8|j;
    }
    for (int i=160;i<256;i++) {
    	j=255*(i-160)/(256-160);
    	firePal[i]=j<<16|255<<8|255;
    }
    double d;
    for (int i=0;i<256;i++) { //alpha
        d=32.0*Math.log(i+1)/Math.log(2.0);
        j=(int)d; j=(j>255)?255:j;
        firePal[i]|=j<<24;
    }
  }

  
  private void iniBlackFire() {
	int j;
    for (int i=0;i<=40;i++) { // blue halo
    	firePal[i]=128*i/40;
    }	  
    for (int i=160;i<256;i++) { // another blue halo
    	j=255*(i-160)/(256-160);
    	firePal[i]=j;
    }
    double d;
    for (int i=0;i<256;i++) { //alpha
        d=32.0*Math.log(i+1)/Math.log(2.0);
        j=(int)d; j=(j>255)?255:j;        
        firePal[i]|=j<<24;
    }         
  }



  private void mapPal(int[] line,int[] drline) {  	
    for (int i=0;i<drline.length;i++)          	
      drline[i]=firePal[line[i]];
  }

  private void mapPalInv(int[] line,int[] drline) {  	
    for (int i=0;i<drline.length;i++)
      drline[i]=firePal[line[drline.length-i-1]];
  }

	public int[] getLine() {
		mapPalInv(linea,dline);//update dline
		return dline;
	}

	public int getLength() {
		return WIDTH;
	}

	
	public String getLineString() {
		String out = "";
		for (int i=0;i<WIDTH;i++) out+=(linea[i]==0)?"0":"1";
		return out;
	}
	
	public void setLine(String s) {
		for (int i=0;i<WIDTH;i++) {
			linea[i]=s.substring(i,i+1).equals("0")?0:255;
		}
	}
	
	/** Each client generates its own line.  This is horizontal
	 * In this case, it will be random.
 	*/
	void genLine() {
		Random  r=new Random();
		int xx,i,j,k;
		
		for (i=0;i<Fire.WIDTH;i+=2) { // puntos blancos aleatorios
			linea[i]=255*r.nextInt(2);
		}
		if (r.nextInt(100)==0) { // se apaga la linea ...
			for (i=0;i<Fire.WIDTH;i++) linea[i]=0;
		}
		if (r.nextInt(10)==0) { // un puntazo de vez en cuando
			j=r.nextInt(Fire.WIDTH-5);
			//k=255*r.nextInt(2);
			for (xx=0;xx<5;xx++) linea[j+xx]=255;
		}      

		update();
	}
	
	void update() {
		int xx,i,j,k;
		
		for (i=1;i<Fire.HEIGHT;i++) {
			for (xx=1;xx<Fire.WIDTH-1;xx++) {
				j=xx+Fire.WIDTH*i;
				k=xx+Fire.WIDTH*(i-1);
				linea[j]=linea[j]/4+linea[k-1]/4+linea[k]/4+linea[k+1]/4;
				if (linea[j]>255) linea[j]=255;
			}
		}		
	}
	
	/** If the share the same ID, the should represent the same object,
	  * since the ID should be defined uniquely.
	  */
	public boolean equals(Object obj) {
		if (obj.getClass().isInstance(this)) {
			Fire ff = (Fire) obj;
			if (ff.getID() == id) return true;
		} 
		return false;
	}

}