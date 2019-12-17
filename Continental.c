#include <windows.h>    /* Include file for Win32 time functions */
#include <formatio.h>
#include <ansi_c.h>
#include <utility.h>
#include <userint.h>
#include "NOxCommunication.h"
#include "Bus Monitor.uir"

#define SIZE 50

void Assign_Box(int ArbIDCount);
void Print_Out(unsigned long ArbID, unsigned char *DataBuf);
int CVICALLBACK QuitCallBack(int panel, int control, int event, void *callbackdata, int eventData1, int eventData2);
int CVICALLBACK StartCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int CVICALLBACK StopCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);

/*Global Variables*/
int iErrorCode = 0;
int gUIRPanelHandle;
int NumBytes;
int gLoop;
int ArbIDCount = 0;
unsigned long ArbID;
unsigned char PortNumber = 0;
unsigned char DataBuf[8];
char ArbIDMemory[SIZE];
char ErrMsg[2048];
char gMessageString[SIZE];
char gMessageString2[SIZE];

ErrMsg[0] = 0;
NumBytes = 0;
DataBuf[0] = '0';

int main()
{
    int i,j,k,a;
    int index;
    int ArbIDScan[SIZE];
    double dInitialize;

    gUIRPanelHandle = LoadPanel (0, "Bus Monitor.uir", PANEL); /*initialize panel*/
    SetActivePanel(gUIRPanelHandle);
    DisplayPanel(gUIRPanelHandle);
    ProcessSystemEvents();
    SetPanelAttribute(gUIRPanelHandle, ATTR_LEFT, 550); /*offset UIR from left*/
    SetPanelAttribute(gUIRPanelHandle, ATTR_TOP, 275);  /*offset UIR from the top*/

    SetCtrlAttribute(gUIRPanelHandle, PANEL_LED_1, ATTR_OFF_COLOR, VAL_BLACK); /*colors of LEDS in ON/OFF state*/
    SetCtrlAttribute(gUIRPanelHandle, PANEL_LED_1, ATTR_ON_COLOR, VAL_GREEN);
    SetCtrlAttribute(gUIRPanelHandle, PANEL_LED_2, ATTR_OFF_COLOR, VAL_BLACK);
    SetCtrlAttribute(gUIRPanelHandle, PANEL_LED_1, ATTR_ON_COLOR, VAL_RED);
    SetCtrlVal(gUIRPanelHandle, PANEL_LED_1, 0);    /*setting LEDs to start OFF*/
    SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 0);

    Restart:
    ArbIDCount = 0;
    dInitialize = 0;
    dInitialize = Timer();
    //int CanOpen(PortNumber, BaudRate, ReadQLength, StandardComp, StandardMask, ExtendedComp, ExtendMask); /*Definition of CanOpen*/
    iErrorCode = CanOpen(PortNumber, 500000, 50, 0, 0x0, 0x0, 0);
    iErrorCode = CanReadNOx(ErrMsg, PortNumber, 1, Databuf, &NumBytes, -1, &ArbID , 10, 5000);

    if(iErrorCode)  /*Reset CAN when error occurs*/
    {
        CanReset(0,PortNumber);
        CanClose(PortNumber);
        iErrorCode = CanOpen(PortNumber, 500000, 50, 0, 0x0, 0x0, 0);
    }

    if(Timer() - dInitialize > 3.00)    /*Power Error*/
    {
        DebugPrintf("Error: No Power\n"); /*Power error occurs when CAN does not respond in 3 seconds or more*/
        SetCtrlAttribute(gUIRPanelHandle, PANEL_MSG_BOX, ATTR_TEXT_BGCOLOR, VAL_RED);
        sprintf(gMessageString,"Error: No Power\n");
        SetCtrlVal(gUIRPanelHandle, PANEL_MSGBOX, gMessageString);  /*Display Error Message in UIR Message Box*/
        strcpy(gMessageString, "");
        SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 1);    /*Toggle LED ON*/
        Delay(2.0);
        return;
    }

    else if(ArbID == 0)     /*USB Error*/
    {
        DebugPrintf("Error: No USB Detected\n");
        SetCtrlAttribute(gUIRPanelHandle, PANEL_MSGBOX, ATTR_TEXT_BGCOLOR, VAL_YELLOW);
        sprintf(gUIRPanelHandle, "Error: No USB Detected\n");
        SetCtrlVal(gUIRPanelHandle, PANEL_MSGBOX, gMessageString);  /*Display Error Message on UIR Message Box*/
        strcpy(gMessageString, "");
        SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 1); /*Toggle LEDs ON and OFF*/
        Delay(1.0);
        SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 0);
        Delay(0.5);
    }

    for(k = 1; k<SIZE; k++)     /*Scanning for ArbIDs*/
    {
        iErrorCode = CanReadNOx(ErrMsg, PortNumber, 1, DataBuf, &NumBytes, -1, &ArbID, 10, 5000);
        ArbIDScan[k] = ArbID;
    }

    for(i = 2; i<SIZE; i++)     /*Storing unique ArbIDs*/
    {
        ArbIDMemory[1] = ArbIDScan[1];
        for(j = 1; j<i; j++)
        {
            if(ArbIDScan[i] == ArbIDScan[j])
                break;
        }

        if(i == j)
        {
            ArbIDMemory[i]=ArbIDScan[i];
            ArbIDCount ++;
        }
    }

    for(i = 1; i<SIZE; i++)     /*Arranging ArbIDs in ascending order*/
    {
        for(j = 1; j<i; j++)
        {
            if((ArbIDMemory[i] != 0) && (ArbIDMemory[j] > ArbIDMemory[i]))
            {
                a = ArbIDMemory[i];
                ArbIDMemory[i] = ArbIDMemory[j];
                ArbIDMemory[j] = a;
            }
        }
    }

    do
    {
        if(gLoop)unsigned long ArbID;
        {
            ProcessSystemEvents();
            SetCtrlVal(gUIRPanelHandle, Panel_LED_2, 0);    /*LEDs in UIR running status*/
            SetCtrlVal(gUIRPanelHandle, Panel_LED_1, 1);
            Assign_Box(ArbIDCount);

            if(Timer() - dInitialize > 10.00)   /* Restart and Reset CAN device because it will get stuck on single ArbID*/
            {
                CanReset(0, PortNumber);
                CanClose(PortNumber);
                iErrorCode = CanOpen(PortNumber, 500000, 50, 0, 0x0, 0x0, 0);
                for(i = 0; i<SIZE; i++)
                {
                    ArbIDMemory[i] = 0;     /*Reset ArbIDMemory array*/
                }
                goto Restart;
            }
        }/*End of gLoop*/

        ProcessSystemEvents();
    }while(gQuit == 0); /*End of do while*/

End_of_function:
    return 0;

} /*End of main*/

void Assign_Box(int ArbIDCount)
{
    int i, j, a;

    for(i = 1: i < ArbIDCount + 1 ; i++)
    {
        iErrorCode = CanReadNOx(ErrMsg, Portnumber, 1, DataBuf, &NumBytes, -1, &ArbID, 10, 5000);
        ArbIDMemory[i] = ArbID;
        sprintf(gMessageString, "0x%X\n", ArbID);
        DebugPrintf("0x%X    ", ArbID);
        SetCtrlVal(gUIRPanelHandle, gUIRArbIDBoxes[i], gMessageString);
        Print_out(ArbID, DatBuf);
        SetCtrlVa;(gUIRPanelHandle, gUIRDatabufBoxes[i], gMessageString2);
        strcpy(gMessageString2, "");
        ProcessSystemEvents();
    }
    return;
}/*End of Assign_Box*/

void Print_out(unsigned long ArbID, unsigned char *DataBuf)
{
    int j;
    DebugPrintf("0x%X    ", ArbID);
    sprintf(gMessageString2, "%02X", DataBuf[0]);
    for(j = 1; j<8; j++)
    {
        sprintf(gMessageString2, "%s %02X", gMessageString2, DataBuf[j]);
    }

    sprintf(gMessageString2, "%s\n", gMessageString2);
    DebugPrintf("%s\n", gMessageString2);
    return;
}/*End of Print_out*/

int CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
    switch(event)
    {
        case EVENT_COMMIT:
            gQuit = 1;
            ProcessSystemEvents();
            QuitUserInterface(0);
            break;
        default:
            DebugPrintf("UIRQuitButton event = %d", event);     /*Display events of button in Debugger*/
            break;
    }
    return 0;
}/*End of QuitCallback*/

int CVICALLBACK StartCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
    switch(event)
    {
        case EVENT_COMMIT:
            gLoop = 1;
            SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 0);
            SetCtrlVal(gUIRPanelHandle, PANEL_LED_1, 1);
            SetCtrlAtrribute(gUIRPanelHandle, PANEL_MSGBOX, ATTR_TEXT_BGCOLOR, VAL_WHITE);
            sprintf(gMessageString, "Continued\n");
            SetCtrlVal(gUIRPanelHandle, PANEL_MSGBOX, gMessageString);
            ProcessSystemEvents();
            break;
        default:
            DebugPrintf("UIRGreenButton event = %d %d\n", event, gLoop);    /*Display events of button in Debugger and status of gLoop*/
            break;
    }
    return 0;
}/*End of StartCallback*/

int CVICALLBACK StopCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
    switch(event)
    {
        case EVENT_COMMIT:
            gLoop = 0;
            SetCtrlVal(gUIRPanelHandle, PANEL_LED_1, 0);
            SetCtrlVal(gUIRPanelHandle, PANEL_LED_2, 1);
            SetCtrlAtrribute(gUIRPanelHandle, PANEL_MSGBOX, ATTR_TEXT_BGCOLOR, VAL_RED);
            sprintf(gMessageString, "Stopped\n\n\n\n");
            SetCtrlVal(gUIRPanelHandle, PANEL_MSGBOX, gMessageString);
            ProcessSyetemEvents();
            break;
        default:
            DebugPrintf("UIRRedButton event = %d %d\n", event, gLoop);      /*Display events of button in Debugger and status of gLoop*/
            break;
    }
    return 0;
}/*End of StopCallback */

