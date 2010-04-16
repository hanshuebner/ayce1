import com.cycling74.max.*;
import com.cycling74.jitter.*;
import java.net.*;
import java.io.IOException;

public class matrix_udp_raw extends MaxObject
{
	
    public int port = 6038;					// default port number
    public String host = "10.31.217.218";			// "localhost" - change this to the desired ip address
    private InetAddress servAddr;
    //	private int header[] = new int[]{4, 1, 220, 74, 1, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 243, 0, 0, 0, 2, 240, 255};
    private int header[] = new int[]{4, 1, 220, 74, 1, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 243, 0, 0, 0, 2, 0, 0};
    private int packSize = 537;
    private byte[] buffer = new byte[packSize];	
    private DatagramSocket sock = null;
    private String PortArray[] = new String[] {"10.31.218.156", "10.31.217.234", "10.31.218.10", "10.31.218.182", "10.31.218.178", "10.31.218.52", "10.31.218.228", "10.31.218.192", "10.31.218.206", "10.31.218.255", "10.31.218.230", "10.31.217.218"};
    private int dmxCount  = 12;
    // private byte header[] = new byte[] {0x04 ,0x01 ,0xDC ,0x4A ,0x01 ,0x00 ,0x08 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x01 ,0xF3 ,0x00 ,0x00 ,0x00 ,0x02 ,0xF0 ,0xFF};

    JitterMatrix jm = new JitterMatrix();
    private static final String[] INLET_ASSIST = new String[]{
        "inlet 1 help"
    };
    private static final String[] OUTLET_ASSIST = new String[]{
        "outlet 1 help"
    };
	
    public matrix_udp_raw(Atom[] args)
    {
        declareInlets(new int[]{DataTypes.ALL});
        declareOutlets(new int[]{DataTypes.ALL});
		
        setInletAssist(INLET_ASSIST);
        setOutletAssist(OUTLET_ASSIST);

        if(args.length>0) {
            if(args[0].isInt())
                port = args[0].getInt();		// argument sets port number
            else {
                error("UdpSendRaw: argument should be port number (int)\n--- setting to default port");
                port = 6038;
            }
        }
        post("UdpSendRaw: sending to "+host+" on port "+port);	

    }

    private void initSocket(){
        if ( sock == null ){
            try {  sock =  new DatagramSocket(); }
            catch(IOException e) {
                System.err.println(e);
            }
        }
    }	

    public void bang()
    {
    }

    public void inlet(int i)
    {
    }

    public void inlet(float f)
    {
    }


    public void list(Atom[] list)
    {
    }

    public void jit_matrix( String s )
    {
        jm.frommatrix(s);

        for (int dmxUniverse = 0; dmxUniverse < dmxCount; dmxUniverse++) {
            for (int portNumber = 0; portNumber < 2; portNumber++) {
                for (int i = 0; i < header.length; i++) {
                    buffer[i] = (byte)header[i];
                }
                buffer[16] = (byte)(portNumber+1);

                //  int dim[] = jm.getDim();
                int theTmpX = 24 * dmxUniverse + 12 * portNumber;
                int theXoffset = ( theTmpX % 96 );
                int theYoffset = ( dmxUniverse / 4 ) * 6;

                for (int panY=0; panY<6; panY++) {
                    for(int panX=0; panX<12; panX++) {
                        int matrixX = theXoffset + panX;
                        int matrixY = theYoffset + panY;
                        int myArray[] = jm.getcell2dInt( matrixX, matrixY );
                        int pointerR = 1;
                        if ( panX < 6 ) {
                            pointerR = ( panX + ( 5 - panY ) * 6  ) * 3 + header.length;
                        } else {
                            pointerR = ( 36 + ( panX - 6 ) + ( ( 5 - panY ) * 6 ) ) * 3 + header.length;
                        }
							
							
                        buffer[pointerR] = (byte)myArray[1];
                        buffer[pointerR+1] = (byte)myArray[2];
                        buffer[pointerR+2] = (byte)myArray[3];
                    }
                }
                initSocket();
				
                try {
                    host = PortArray[dmxUniverse];
                    servAddr = InetAddress.getByName(host);
                    // create UDP-Packet to send
                    DatagramPacket packet = new DatagramPacket(buffer, packSize, servAddr, port);
                    sock.send(packet);
                }
                catch(IOException e) {
                    System.err.println(e);
                }
            }
        }		
    }
}

