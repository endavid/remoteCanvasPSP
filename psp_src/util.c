#include "std.h"
#include "util.h"
#include "pg_redux.h"

#include "my_socket.h"

/////////////////////////////////////////////////////

void my_print_init()
{
    // wipe out old logfile
    int fdLog = sceIoOpen("ms0:/err.txt", PSP_O_CREAT|PSP_O_RDWR|PSP_O_TRUNC, 0777);
    sceIoClose(fdLog);
}

void my_print(const char* str)
{
    int fdLog = sceIoOpen("ms0:/err.txt", PSP_O_WRONLY|PSP_O_APPEND, 0777);
    if (fdLog > 0)
    {
        sceIoWrite(fdLog, (void*)str, strlen(str));
        sceIoClose(fdLog);
    }
}

void my_printn(const char* str1, int val, const char* str2)
{
    char szT[32];
    my_print(str1);
    sprintf(szT, "%x", val);
    my_print(szT);
    my_print(str2);
}

/////////////////////////////////////////////////////
// very simple UI for picking from a short list

void my_initpicker(PICKER* pickerP, const char* szTitle)
{
    strncpy(pickerP->szTitle, szTitle, MAX_PICK_TITLE);
    pickerP->szTitle[MAX_PICK_TITLE] = '\0';
    pickerP->pick_count = 0;
    pickerP->pick_start = -1;
}

bool my_addpick(PICKER* pickerP, const char* szBig, const char* szFinePrint, u32 userData)
{
    PICKER_INFO* pickP;
    if (pickerP->pick_count >= MAX_PICK)
        return false; // no room
        // REVIEW: in future provide a smaller font version with more slots
    pickP = &(pickerP->picks[pickerP->pick_count++]);

    strncpy(pickP->szBig, szBig, MAX_PICK_MAINSTR);
    pickP->szBig[MAX_PICK_MAINSTR] = '\0';
    if (szFinePrint == NULL)
        szFinePrint = "";
    strncpy(pickP->szFinePrint, szFinePrint, MAX_PICK_FINEPRINT);
    pickP->szFinePrint[MAX_PICK_FINEPRINT] = '\0';
    pickP->userData = userData;
    return true;
}

int my_loadpicks_fromfile(PICKER* pickerP, const char* szFile, bool bParseIP)
    // returns number of entries read
{
    FILE* pf = fopen(szFile, "rt");
    if (pf == 0)
        return 0;
    char szLine[128];
    int nAdded = 0;
    while (fgets(szLine, sizeof(szLine), pf) != NULL)
    {
        // CR-LF will cause blank lines
        if (szLine[0] == '\0' || szLine[0] == ';')
            continue;   // skip

        // format is "BIG_STRING;FinePrint"
        char* szFinePrint = strchr(szLine, ';');
        if (szFinePrint != NULL)
            *szFinePrint++ = '\0';
        u32 userData = 0;
        if (bParseIP)
        {
            // parse szLine for ?.?.?.? IP address
            userData = sceNetInetInetAddr(szLine);
            if (userData == 0)
                continue;   // bad, skip it
        }
        // only first 5 will fit
        if (my_addpick(pickerP, szLine, szFinePrint, userData))
            nAdded++;
    }
    fclose(pf);
    return nAdded;
}

/////////////////////////////////////////////////////
// Actual UI

// pixel coordinates (use pgPrint4AtPixel)
#define Y_TITLE 0
#define X_TITLE 0

#define Y_PICK(i)  (32+8+48*(i))
#define X_PICK (2*8*4)
#define STR_PICK "=>"
#define Y_PICK_FINEPRINT(i) (Y_PICK(i)+32) // after big font
#define X_PICK_FINEPRINT (120)

int my_picker(const PICKER* pickerP)
{
    int iPick = pickerP->pick_start;
    u32 buttonsLast = 0;
    int bRedraw = 1;

    if (pickerP->pick_count == 0)
        return -1; // nothing to pick

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(0); // digital

    while (1)
    {
        int i;
        SceCtrlData pad;
        u32 buttonsNew;

        if (pickerP->pick_count == 1)
        {
            // auto-pick if only one
            iPick = 0;
            break;
        }

        if (bRedraw)
        {
            pgFillvram(0);
            pgPrint4AtPixel(X_TITLE, X_TITLE, COLOR_GREY50, pickerP->szTitle);
            for (i = 0; i < pickerP->pick_count; i++)
            {
                PICKER_INFO const* pickP = &pickerP->picks[i];
                u32 color1 = COLOR_GREY25;
                u32 color2 = COLOR_GREY12;
                int y2;
                if (i == iPick)
                {
                    color1 = COLOR_GREY75;
                    color2 = COLOR_GREY50;
                    pgPrint4AtPixel(0, Y_PICK(i), COLOR_WHITE, STR_PICK);
                }
                pgPrint4AtPixel(X_PICK, Y_PICK(i), color1, pickP->szBig);
                y2 = Y_PICK_FINEPRINT(i);
                if (i != MAX_PICK-1)
                    y2 += 2;    // space it out a little
                pgPrint1AtPixel(X_PICK_FINEPRINT, y2, color2, pickP->szFinePrint);
            }
            pgScreenFlipV();
            bRedraw = 0;
        }

        // handle up/down inputs
        sceCtrlReadBufferPositive(&pad, 1); 
        buttonsNew = pad.Buttons & ~buttonsLast;
        buttonsLast = pad.Buttons;
        if (buttonsNew & PSP_CTRL_DOWN)
        {
            iPick++;
            if (iPick >= pickerP->pick_count)
                iPick = pickerP->pick_count-1;
            bRedraw = 1;
        }
        else if (buttonsNew & PSP_CTRL_UP)
        {
            iPick--;
            if (iPick < 0)
                iPick = 0;
            bRedraw = 1;
        }
        else if (buttonsNew & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS))
        {
            if (iPick >= 0 && iPick < pickerP->pick_count)
                break; // picked !
        }
        else if (buttonsNew & PSP_CTRL_TRIANGLE)
        {
            iPick = -1; // cancel
            break;
        }
    }

    // show_confirmation: show final selection for a short time
    pgFillvram(0);
    pgPrint4AtPixel(X_TITLE, Y_TITLE, COLOR_GREY50, pickerP->szTitle);
    if (iPick != -1)
        pgPrint4AtPixel(X_PICK, Y_PICK(iPick), COLOR_WHITE, pickerP->picks[iPick].szBig);
    else
        pgPrint4AtPixel(X_PICK, Y_PICK(2), COLOR_WHITE, "[Cancel]");
    pgScreenFlipV();
    pgWaitVn(50);

    return iPick;
}

/////////////////////////////////////////////////////
// Further utils by Hawk/FB
//

# define MIDSCREEN 120

// Picks an IP address - int[4]
int IP_picker(int* address)
{
    u32 buttonsLast = 0;
    int bRedraw = 1;
    int iPick = 0;
    int ipaddr[4];

    if (address == NULL)
        return -1; // nowhere to store the IP!

    // copy the address
    for (iPick = 0;iPick<4;iPick++) ipaddr[iPick]=address[iPick];
    iPick = 0;
    
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(0); // digital

    char number[5];
    while (1)
    {
        int i;
        SceCtrlData pad;
        u32 buttonsNew;


        if (bRedraw)
        {
            pgFillvram(0);
            pgPrint4AtPixel(X_TITLE, X_TITLE, COLOR_GREY50, "Server IP");
            for (i = 0; i < 4; i++)
            {
                u32 color1 = COLOR_GREY25;
                if (i == iPick)
		{
                    color1 = COLOR_GREY75;
                    // underline selected number
                    pgPrint2AtPixel(i*16*4+32, MIDSCREEN+16, COLOR_WHITE, "---");
                }
                sprintf(number,"%3d",ipaddr[i]);
                if (i < 3) {
		    number[3]='.';
		    number[4]=0;
                }
                pgPrint2AtPixel(i*16*4+32, MIDSCREEN, color1, number);
            }
            pgScreenFlipV();
            bRedraw = 0;
        }

        // handle inputs
        sceCtrlReadBufferPositive(&pad, 1);
        buttonsNew = pad.Buttons & ~buttonsLast;
        buttonsLast = pad.Buttons;
        if (buttonsNew & PSP_CTRL_RIGHT)
        {
            iPick++;
            if (iPick >= 4)
                iPick = 0;
            bRedraw = 1;
        }
        else if (buttonsNew & PSP_CTRL_LEFT)
        {
            iPick--;
            if (iPick < 0)
                iPick = 3;
            bRedraw = 1;
        }
        else if (buttonsLast & PSP_CTRL_UP) {
             ipaddr[iPick]++;
             if (ipaddr[iPick]>255) ipaddr[iPick]=0;
             bRedraw = 1;
        }
        else if (buttonsLast & PSP_CTRL_DOWN) {
             ipaddr[iPick]--;
             if (ipaddr[iPick]<0) ipaddr[iPick]=255;
             bRedraw = 1;
        }
        else if (buttonsNew & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS))
        {
                break; // picked !
        }
        else if (buttonsNew & PSP_CTRL_TRIANGLE)
        {
            iPick = -1; // cancel
            break;
        }
    }

    // show_confirmation: show final selection for a short time
    pgFillvram(0);
    pgPrint4AtPixel(X_TITLE, Y_TITLE, COLOR_GREY50, "Server IP");
    if (iPick != -1) {
	int i;
	for (i=0;i<4;i++) address[i]=ipaddr[i];
    }
    
    for (iPick = 0; iPick<4; iPick++) {
        sprintf(number,"%3d",address[iPick]);
    	if (iPick < 3) {
	    number[3]='.';
	    number[4]=0;
        }
        pgPrint2AtPixel(iPick*16*4+32, MIDSCREEN, COLOR_WHITE, number);
    }
    pgScreenFlipV();
    pgWaitVn(50);

    return 0;
}

