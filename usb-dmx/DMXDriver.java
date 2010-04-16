import java.io.*;

public class DMXDriver 
{
    static private FileOutputStream out = null;
    static final private byte DLE = 0x55;
    static final private byte CMD_START = 0x00;

    static private void transmitStreams(byte[][] streams, int count)
    {
        byte[] buf = new byte[32 * count + 18]; // allocate double space for possible DLE escaping + header
        int output_pos = 0;
        buf[output_pos++] = DLE; // DLE
        buf[output_pos++] = 0x00; // CMD 0 => start DMX frame
        // Send 0 byte on all channels to start DMX frame
        for (int i = 0; i < 16; i++) {
            buf[output_pos++] = 0x00;
        }
        // Send 3 0 bytes as the first LED seems not to be addressable
        for (int i = 0; i < 16; i++) {
            buf[output_pos++] = 0x00;
        }
        for (int i = 0; i < 16; i++) {
            buf[output_pos++] = 0x00;
        }
        for (int i = 0; i < 16; i++) {
            buf[output_pos++] = 0x00;
        }
        for (int pos = 0; pos < count; pos++) {
            byte slice[] = new byte[16];
            for (int channel = 0; channel < 16; channel++) {
                slice[channel] = streams[channel][pos];
            }
            for (byte bit = 0; bit < 8; bit++) {
                byte b = (byte) (slice[0] & 1
                                 | ((slice[1] & 1) << 1)
                                 | ((slice[2] & 1) << 2)
                                 | ((slice[3] & 1) << 3)
                                 | ((slice[4] & 1) << 4)
                                 | ((slice[5] & 1) << 5)
                                 | ((slice[6] & 1) << 6)
                                 | ((slice[7] & 1) << 7));
                buf[output_pos++] = b;
                if (b == DLE) {
                    buf[output_pos++] = b;
                }
                b = (byte) (slice[8] & 1
                            | ((slice[9] & 1) << 1)
                            | ((slice[10] & 1) << 2)
                            | ((slice[11] & 1) << 3)
                            | ((slice[12] & 1) << 4)
                            | ((slice[13] & 1) << 5)
                            | ((slice[14] & 1) << 6)
                            | ((slice[15] & 1) << 7));
                buf[output_pos++] = b;
                if (b == DLE) {
                    buf[output_pos++] = b;
                }
                for (int channel = 0; channel < 16; channel++) {
                    slice[channel] >>= 1;
                }
            }
        }
        try {
            out.write(buf, 0, output_pos);
        }
        catch (IOException x) {
            System.out.println("IO exception");
        }
    }

    public static void sendTestPattern()
    {
        byte streams[][] = {{0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F},
                            {0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F},
                            {2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 },
                            {3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 },
                            {4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 , 4 },
                            {5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 , 5 },
                            // {6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 },
                            {0x55, 0x55, 0x55, 0x55,
                             (byte) 0xAA, (byte) 0xAA, (byte) 0xAA, (byte) 0xAA,
                             (byte) 0x80, 0x40, 0x20, 0x10,
                             0x08, 0x04, 0x02, 0x01},
                            {7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 , 7 },
                            {8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 , 8 },
                            {9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 },
                            {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
                            {11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11},
                            {12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
                            {13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13},
                            {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                            {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}};
        transmitStreams(streams, 3);
    }

    public static void setColor(int channel, int r, int g, int b) {
    }

    public static void bunt() {
        byte streams[][] = new byte[16][144*3];
        for (int i = 0; i < 144; i++) {
            streams[0][i*3 + (i % 3)] = (byte) 0xff;
        }
        transmitStreams(streams, 144*3);
    }

    public static void speed() {
        byte streams[][] = new byte[16][144*3];
        System.out.println("start");
        for (int i = 0; i < 100; i++) {
            transmitStreams(streams, 144*3);
        }
        System.out.println("end");
    }

    public static void pulse() {
        byte streams[][] = new byte[16][144*3];
        for (int intensity = 0; intensity < 256; intensity++) {
            for (int i = 0; i < 144; i++) {
                streams[0][i*3 + (i % 3)] = (byte) intensity;
            }
            transmitStreams(streams, 144*3);
        }
        for (int intensity = 255; intensity >= 0; intensity--) {
            for (int i = 0; i < 144; i++) {
                streams[0][i*3 + (i % 3)] = (byte) intensity;
            }
            transmitStreams(streams, 144*3);
        }
    }

    public static void walk() {
        byte streams[][] = new byte[16][144*3];
        for (int i = 0; i < 144; i++) {
            streams[0][i*3] = (byte) 255;
            streams[0][i*3 + 1] = (byte) 255;
            streams[0][i*3 + 2] = (byte) 255;
            transmitStreams(streams, 144*3);
        }
    }

    public static void flash() {
        while (true) {
            byte streams[][] = new byte[16][144*3];
            for (int i = 0; i < 144; i++) {
                streams[0][i*3] = (byte) 255;
                streams[0][i*3 + 1] = (byte) 255;
                streams[0][i*3 + 2] = (byte) 255;
            }
            transmitStreams(streams, 144*3);
            for (int i = 0; i < 144; i++) {
                streams[0][i*3] = (byte) 0;
                streams[0][i*3 + 1] = (byte) 0;
                streams[0][i*3 + 2] = (byte) 0;
            }
            transmitStreams(streams, 144*3);
        }
    }

    public static void line() {
        while (true) {
            for (int line = 0; line < 6; line++) {
                byte streams[][] = new byte[16][144*3];
                for (int i = line * 6; i < line * 6 + 6; i++) {
                    streams[0][i*3] = (byte) 255;
                    streams[0][i*3 + 1] = (byte) 255;
                    streams[0][i*3 + 2] = (byte) 255;
                    streams[0][(i + 36) * 3] = (byte) 255;
                    streams[0][(i + 36) * 3 + 1] = (byte) 255;
                    streams[0][(i + 36) * 3 + 2] = (byte) 255;
                }
                transmitStreams(streams, 144*3);
            }
            for (int line = 0; line < 6; line++) {
                byte streams[][] = new byte[16][144*3];
                for (int i = line * 6; i < line * 6 + 6; i++) {
                    streams[0][72*3 + i*3] = (byte) 255;
                    streams[0][72*3 + i*3 + 1] = (byte) 255;
                    streams[0][72*3 + i*3 + 2] = (byte) 255;
                    streams[0][72*3 + (i + 36) * 3] = (byte) 255;
                    streams[0][72*3 + (i + 36) * 3 + 1] = (byte) 255;
                    streams[0][72*3 + (i + 36) * 3 + 2] = (byte) 255;
                }
                transmitStreams(streams, 144*3);
            }
        }
    }

    public static void step() {
        try {
            BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
            for (int i = 0; i < 144; i++) {
                byte streams[][] = new byte[16][144*3];
                streams[0][i*3] = (byte) 0xff;
                transmitStreams(streams, 144*3);
                System.out.println("At " + i);
                String s = in.readLine();
            }
        }
        catch (IOException e) {
            System.out.println("Error while reading: " + e.getMessage());
        }
    }

    public static void main (String argv[])
    {
        try {
            out = new FileOutputStream(argv[0]);
            // bunt();
            // pulse();
            // walk();
            // flash();
            line();
            // step();
            // speed();
        }
        catch (FileNotFoundException x) {
            System.out.println("file not found");
        }
    } 
}