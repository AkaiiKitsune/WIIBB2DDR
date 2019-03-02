#pragma once
#include "stdafx.h"

#include <windows.h>
#include <bthsdpdef.h>
#include <bthdef.h>
#include <BluetoothAPIs.h>
#include <strsafe.h>
#include <iostream>
#include <conio.h>

#include <stdio.h> /* for printf */
#include "wiiuse.h" /* for wiimote_t, classic_ctrl_t, etc */

int mainLoop();
int connection(int disconnect);
void handle_event(struct wiimote_t *wm);
void handle_read(struct wiimote_t *wm, byte *data, unsigned short len);
void handle_ctrl_status(struct wiimote_t *wm);
void handle_disconnect(wiimote *wm);
short any_wiimote_connected(wiimote **wm, int wiimotes);
DWORD ShowErrorCode(const wchar_t  *msg, DWORD dw);
_TCHAR * FormatBTAddress(BLUETOOTH_ADDRESS address);
int loopThroughBTDevices(int nRadios, int &nPaired, HANDLE* hRadios, char remove);


#ifndef WIIUSE_WIN32
#include <unistd.h> /* for usleep */
#endif
#define MAX_WIIMOTES 4
int firstRun = 0;
double frontLeftCal, frontRightCal, backLeftCal, backRightCal;
double frontLeft, frontRight, backLeft, backRight;

#pragma comment(lib, "Bthprops.lib")


int loopErrorMax = 10;