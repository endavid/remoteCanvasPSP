// WiFi Multi-Test app
// .03 - user mode thread refactoring
// with thanks to 'benji'

// see "WiFi Simple .03" for a simpler example
//
// "Remote Canvas" 0.6 by Hawk/FB
//
#include "std.h"

#include "pg_redux.h"
#include "util.h"

#include "nlh.h" // net lib helper

#define WIDTH 480
#define HEIGHT 272
#define CMAX2_X 29
#define CMAX2_Y 16
#define MAX_CHARS 30*19

#define MAX_USERS 5


////////////////////////////////////////////////////

//NOTE: kernel mode module flag and kernel mode thread are both required
PSP_MODULE_INFO("WIFI_TEST_APP", 0x1000, 1, 1); /* 0x1000 REQUIRED!!! */
PSP_MAIN_THREAD_ATTR(0); /* 0 REQUIRED!!! */
PSP_MAIN_THREAD_STACK_SIZE_KB(32); /* smaller stack for kernel thread */

static void DoInetNetTest(); // web access
int user_main(SceSize args, void* argp);
int receiveThread(SceSize args, void *argp); // thread that listens for data

u32 colors[MAX_USERS+1] = {
    RGB(255,255,255),
    RGB(255,0,0),
    RGB(0,255,0),
    RGB(0,0,255),
    RGB(255,255,0),
    RGB(0,255,255)};

int server_address[4] = {192, 168, 1, 2};

int main(void)
{
    // kernel mode thread
    pgInit32(); // RGB color version - double buffering

    {
        pgFillvram(0);
        pgPrint4(0,1,COLOR_WHITE,"WiFi MultiTest");
        pgPrint4(0,3,COLOR_GREY50,"Version .03");
        pgPrint(0,18,COLOR_GREY50,"    TelnetD by PspPet");
        pgPrint(0,20,COLOR_GREY50,"    RCanvas 0.6 by Hawk");
        pgScreenFlipV();
        pgWaitVn(10);
    }

    my_print_init();
    my_print("WiFi modules loading\n");

    if (nlhLoadDrivers(&module_info) != 0)
    {
        my_print("Net driver load error\n");
        return 0;
    }

    // create user thread, tweek stack size here if necessary
    SceUID thid = sceKernelCreateThread("User Mode Thread", user_main,
            0x11, // default priority
            256 * 1024, // stack size (256KB is regular default)
            PSP_THREAD_ATTR_USER, NULL);

    // start user thread, then wait for it to do everything else
    sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitThreadEnd(thid, NULL);

    sceKernelExitGame();
    return 0;
}

int user_main(SceSize args, void* argp)
{
    // user mode thread does all the real work

    DoInetNetTest();
    return 0;
}

////////////////////////////////////////////////////
// The tests

static void TestMiniTelnetD(const char* szMyIPAddr);
static void canvasClient(const char* szMyIPAddr);
static void settings(const char* szMyIPAddr);


////////////////////////////////////////////////////
// the main test routine - pick connection, connect, ask which tests to run

static void DoInetNetTest()
{
    u32 err;
    int state;
    char szMyIPAddr[32];
    int connectionConfig = -1;
    int iTest = 0;

    // nlhInit must be called from user thread for DHCP to work
    err = nlhInit();
    my_printn("nlhInit returns ", err, "\n");

    if (err != 0)
    {
        nlhTerm();
        return;
    }

    // enumerate connections
    {
        PICKER pickConn; // connection picker
        int iNetIndex;
        int iPick;

        my_initpicker(&pickConn, "Pick Connection");
        for (iNetIndex = 1; iNetIndex < 100; iNetIndex++) // skip the 0th connection
        {
            char data[128];
            char name[128];
            char detail[128];
            if (sceUtilityCheckNetParam(iNetIndex) != 0)
                break;  // no more
            // my_printn8("config ", (u8)iNetIndex, "\n");
            sceUtilityGetNetParam(iNetIndex, 0, name);
            // my_print(" NAME='"); my_print(name); my_print("'\n");

            sceUtilityGetNetParam(iNetIndex, 1, data);
            strcpy(detail, "SSID=");
            strcat(detail, data);

            sceUtilityGetNetParam(iNetIndex, 4, data);
            if (data[0])
            {
                // not DHCP
                sceUtilityGetNetParam(iNetIndex, 5, data);
                strcat(detail, " IPADDR=");
                strcat(detail, data);
            }
            else
            {
                strcat(detail, " DHCP");
            }

            my_addpick(&pickConn, name, detail, (u32)iNetIndex);

            if (pickConn.pick_count >= MAX_PICK)
                break;  // no more
        }

        if (pickConn.pick_count == 0)
        {
            pgFillvram(0);
            pgPrint4(0,1,COLOR_WHITE, "no connections");
            pgPrint4(0,3,COLOR_WHITE, "please create");
            pgPrint4(0,4,COLOR_WHITE, "connection with");
            pgPrint4(0,5,COLOR_WHITE, "static IP addr");
            pgScreenFlipV();
            pgWaitVn(200);
            goto close_net;
        }

        iPick = my_picker(&pickConn);
        if (iPick == -1)
            goto close_net; // give up
        connectionConfig = (int)(pickConn.picks[iPick].userData);
    }

    my_printn("using connection ", connectionConfig, "\n");

    // try first connection
    err = sceNetApctlConnect(connectionConfig);
    my_printn("sceNetApctlConnect returns ", err, "\n");
    if (err != 0)
        goto close_net;

    state = 0;
    err = sceNetApctlGetState(&state);
    my_printn("getstate: err=", err, ", ");
    my_printn("state=", state, "\n");
    if (err != 0)
        goto close_connection;

    // 4 connected with IP address
    while (1)
    {
        char szT[32];
        int state;

        err = sceNetApctlGetState(&state);
        if (err != 0)
        {
            my_printn("sceNetApctlGetState returns ", err, "\n");
            goto close_connection;
        }
        pgFillvram(0);
        pgPrint4(0,1,COLOR_WHITE, "Connecting");
        if (state >= 0 && state <= 4)
        {
            // cute little status bar
            memset(szT, '+', 10);
            szT[(state+1)*2] = '\0';
        }
        else
            sprintf(szT, "state=%d", state);
        pgPrint4(0,2,COLOR_WHITE, szT);

        pgScreenFlipV();
        pgWaitVn(50);

        // 0 - idle
        // 1,2 - starting up
        // 3 - waiting for dynamic IP
        // 4 - got IP - usable
        if (state == 4)
            break;  // connected with static IP
    }

    // get IP address
    {
        if (sceNetApctlGetInfo(8, szMyIPAddr) != 0)
            strcpy(szMyIPAddr, "unknown IP address");

        my_print("sceNetApctlGetInfo #8: ipaddr=");
        my_print(szMyIPAddr);
        my_print("\n");
    }

    while (1)
    {
        typedef void (*TEST_PROC)(const char* szIPAddr);
        TEST_PROC testProc;

        PICKER pickTest; // room for title + 5 picks
        my_initpicker(&pickTest, "== Pick Test ==");
        my_addpick(&pickTest, "Mini TelnetD", "mini telnet-like test PC->PSP",
                (u32)TestMiniTelnetD);
	my_addpick(&pickTest, "Remote Canvas", "small canvas client",
				(u32)canvasClient);
	my_addpick(&pickTest, "Settings", "Change Server IP", (u32)settings);
        my_addpick(&pickTest, "Exit", "end tests",
                (u32)NULL);

        pickTest.pick_start = iTest;
        iTest = my_picker(&pickTest);

        if (iTest == -1)
            continue;   // ignore triangle - don't exit unless "Exit" picked

        testProc = (TEST_PROC)(pickTest.picks[iTest].userData);

        if (testProc == NULL)
            break;  // EXIT

        // run the test
        (*testProc)(szMyIPAddr);
    }

close_connection:
    err = sceNetApctlDisconnect();
    my_printn("sceNetApctlDisconnect returns ", err, "\n");

close_net:
    nlhTerm();
}

//////////////////////////////////////////////////////////////////////
// Simple test - TCP/IP server - a fake telnetd

static void TestMiniTelnetD(const char* szMyIPAddr)
{
    u32 err;

    // instructions
    {
        pgFillvram(0);
        pgPrint4(0,1,COLOR_WHITE, "Connected");
        pgPrint4(0,3,COLOR_WHITE, "telnet to");
        pgPrint4(0,4,COLOR_WHITE, szMyIPAddr);
        pgPrint4(0,5,COLOR_WHITE, "port 23");
        pgScreenFlipV();
        pgWaitVn(10);
    }

    // mini telnetd-lite server
    {
        SOCKET sockListen;
        struct sockaddr_in addrListen;
        struct sockaddr_in addrAccept;
        int cbAddrAccept;
        SOCKET sockClient;

        sockListen = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
        my_printn("socket returned ", sockListen, "\n");
        if (sockListen & 0x80000000)
            goto done;

        addrListen.sin_family = AF_INET;
        addrListen.sin_port = htons(23);
        addrListen.sin_addr[0] = 0;
        addrListen.sin_addr[1] = 0;
        addrListen.sin_addr[2] = 0;
        addrListen.sin_addr[3] = 0;
            // any

        err = sceNetInetBind(sockListen, &addrListen, sizeof(addrListen));
        my_printn("bind returned ", err, "\n");
        my_printn("  errno=", sceNetInetGetErrno(), "\n");
        if (err)
            goto done;

        err = sceNetInetListen(sockListen, 1);
        my_printn("listen returned ", err, "\n");
        my_printn("  errno=", sceNetInetGetErrno(), "\n");
        if (err)
            goto done;

        // blocking accept (wait for one connection)
        cbAddrAccept = sizeof(addrAccept);
        sockClient = sceNetInetAccept(sockListen, &addrAccept, &cbAddrAccept);
        my_printn("accept returned ", err, "\n");
        my_printn("  errno=", sceNetInetGetErrno(), "\n");
        my_printn("  cb=", cbAddrAccept, "\n");
        if (sockClient & 0x80000000)
            goto done;

        err = sceNetInetSend(sockClient, "Hello from PSP\n\r", 14+2, 0);
        err = sceNetInetSend(sockClient, "type all you want\n\r", 17+2, 0);

        // lame processing loop
        {
            // we draw using the medium pg2 font
            // 30 x 19 characters
            const int maxChars = 30*19;
            char telnet_string[maxChars+1];
            memset(telnet_string, ' ', maxChars); // start with blanks
            telnet_string[maxChars] = '\0';

            int insertPoint = 0;

            while (1)
            {
                int cch;
                cch = sceNetInetRecv(sockClient,
                    (u8*)telnet_string+insertPoint,
                    maxChars-insertPoint, 0);
                if (cch <= 0)
                    break; // usually connection dropped
                insertPoint += cch;
                if (insertPoint >= maxChars)
                    insertPoint = 0;    // start over at top of the screen

                pgFillvram(0);
                pgPrint2(0,0,COLOR_WHITE,telnet_string);
                        // this does line wrap for us
                pgScreenFlipV();
            }
        }
        err = sceNetInetClose(sockClient);
            // REVIEW: shutdown() instead ?
        my_printn("closesocket(client) returned ", err, "\n");

done:
        // done for now
        err = sceNetInetClose(sockListen);
        my_printn("closesocket returned ", err, "\n");
    }
}



////////////////////////////////////////////////////////////////////
// Canvas client
// by Hawk/FB
//
// Protocol:
//  * on connect: 1 (ID number)
//  * commands:
//     /c ID# : Command from the server to change the ID (repeated)
//     /a ID:   Add user
//     /d ID:   Delete user
//     /r ID x y DATA:  User ID sends coordinates x,y and some DATA
//
//  This application will just show the different users at coordinates
//  given by (x,y)
//
//
// History:
//
// 0.6 You can pick the server IP, and others use pixel coordinates
// 0.5 Create a separate thread for receiving
// 0.4 Track users
// 0.3 Simple graphic output
// 0.2 Trasmission loop
// 0.1 Just connects to a fixed address and sends a msg
//
int receiving = 1;
int received = 0;
SOCKET sock;
char msg[MAX_CHARS+1];

static void canvasClient(const char* szMyIPAddr)
{
    u32 err;
	//int bRedraw = 1;

    int port=4004;

    // chat client
    {
        //SOCKET sock;
        struct sockaddr_in addrAsk;
        struct sockaddr_in addrServer;
        //int cbAddrAccept;

		// client socket
        sock = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
        my_printn("socket returned ", sock, "\n");
        if (sock & 0x80000000)
            goto closing;

		addrAsk.sin_family=AF_INET;
		addrAsk.sin_port=0;		
		//addrAsk.sin_addr.s_addr=INADDR_ANY;
		//any
        addrAsk.sin_addr[0] = 0;
        addrAsk.sin_addr[1] = 0;
        addrAsk.sin_addr[2] = 0;
        addrAsk.sin_addr[3] = 0;
		
	// bind the client socket to local address
        err = sceNetInetBind(sock, &addrAsk, sizeof(addrAsk));
        my_printn("bind returned ", err, "\n");
        my_printn("  errno=", sceNetInetGetErrno(), "\n");
        if (err)
            goto closing;
		
		
        // server address
        addrServer.sin_family = AF_INET;
        addrServer.sin_port = htons(port);
        addrServer.sin_addr[0] = server_address[0];
        addrServer.sin_addr[1] = server_address[1];
        addrServer.sin_addr[2] = server_address[2];
        addrServer.sin_addr[3] = server_address[3];

		
        // connect to server
        err = sceNetInetConnect(sock, &addrServer, sizeof(addrServer));
        my_printn("listen returned ", err, "\n");
        my_printn("  errno=", sceNetInetGetErrno(), "\n");
        if (err)
            goto closing;


        err = sceNetInetSend(sock, "1\n\r", 3, 0);
        err = sceNetInetSend(sock, "Hello! I'm a PSP\n\r", 16+2, 0);

        memset(msg, ' ', MAX_CHARS); // start with blanks
        msg[MAX_CHARS] = '\0';

	receiving = 1;
	SceUID thid = sceKernelCreateThread(
	       "listen_thread",
	       receiveThread,
	       32,
	       256*1024,
	       PSP_THREAD_ATTR_USER,
	       0);

	if (thid < 0) { // error
	   goto closing;
	}
	// start thread
	sceKernelStartThread(thid, 0, 0);
        // processing loop
        {

	    SceCtrlData pad;
	    int x=0, y=0;
	    int pressed = 0; // pressed button
	    int nusers=0;
	    int posx[MAX_USERS+1];
	    int posy[MAX_USERS+1];
	    int self=0, who=0;
	    int moved=0;
	    int i;
	    for (i=0;i<=MAX_USERS;i++) {
	    	posx[i]=-1; posy[i]=-1;
    	    }
            while (receiving)
            {
		pressed = 0;
		// this is a blocking receive
		// it should go on a separate thread!
                //int cch;
                //cch = sceNetInetRecv(sock,
                //    (u8*)msg,
                //    maxChars, 0);
                //if (cch <= 0)
                //    break; // usually connection dropped

		//interpret
		if (received) {
		if (msg[0]=='/' && msg[2]==' ') { // command
		  if (msg[1]=='a') { // add user
	   	     nusers++;
	   	     who = msg[3]-'1';
	   	     posx[who]=0; posy[who]=0;
		  } else if (msg[1]=='d') { // delete user
		     nusers--;
		     who = msg[3]-'1';
		     posx[who]=-1; posy[who]=-1;
		  } else if (msg[1]=='c') { // change name
		     self = msg[3]-'1';
		  } else if (msg[1]=='r') { // data
		     who = msg[3]-'1';
		     if (who!=self) {
			sscanf(&msg[5],"%d %d",&posx[who],&posy[who]);
		     }
		  }
		}
		received=0;
		}
		
		moved=0;
                sceCtrlReadBufferPositive(&pad, 1);
                if (pad.Buttons & PSP_CTRL_UP) {
		   if (y>0) y--;
		   moved=1;
		}
		else if (pad.Buttons & PSP_CTRL_DOWN) {
		   if (y<CMAX2_Y) y++;
		   moved=1;
                }
                else if (pad.Buttons & PSP_CTRL_RIGHT) {
		   if (x<CMAX2_X) x++;
		   moved=1;
		}
		else if (pad.Buttons & PSP_CTRL_LEFT) {
		   if (x>0) x--;
		   moved=1;
                }
                else if (pad.Buttons & PSP_CTRL_CROSS) { // mouse left click
                     pressed = 1;
		}
		else if (pad.Buttons & PSP_CTRL_CIRCLE) { // mouse right click
		     pressed = 2;
		}
                else if (pad.Buttons & PSP_CTRL_TRIANGLE) { //exit
                   break;
                }

		//if (moved) {// update position to clients
		 // doing like this, it will make us invisible to
		 // clients that just connected if we don't move
		 sprintf(msg, "/r %d %d %d\n",x<<4,y<<4,pressed); //coord * 16
        	 err = sceNetInetSend(sock, msg, strlen(msg), 0);
        	//}
        	// also, cos the other generate a random line for us
        	// send all the time the signal

                pgFillvram(0);
                
		sprintf(msg,"%d",self);
                pgPrint2(x,y,colors[self],msg);
                for (i=0;i<=MAX_USERS;i++) {
		   if (posx[i]>=0) {
		      sprintf(msg,"%d",i);
		      pgPrint2AtPixel(posx[i],posy[i],colors[i],msg);
		   }
		}
                pgScreenFlipV();
                sceDisplayWaitVblankStart();
            }
        }

closing:
        // done for now
        receiving = 0;
        err = sceNetInetClose(sock);
        my_printn("closesocket returned ", err, "\n");
    }
}


// thread that listens for data
int receiveThread(SceSize args, void *argp) {
    receiving = 1;
    while (receiving) {
        int cch = 1;
        if (received == 0) { // wait till last received command gets interpreted
           cch = sceNetInetRecv(sock,
                    (u8*)msg,
                    MAX_CHARS, 0);
	   received=1;
        }
        if (cch <= 0)
           break; // usually connection dropped
    }
    receiving = 0;
    return 0;
}


// Change the settings
// 0.1 Change server IP
static void settings(const char* szMyIPAddr) {
       IP_picker(server_address);
}

