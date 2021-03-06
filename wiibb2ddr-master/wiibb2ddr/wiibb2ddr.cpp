#include "stdafx.h"
#include "wiibb2ddr.h"



int _tmain(int argc, _TCHAR* argv[])
{
	mainLoop();

	return 0;
}

int mainLoop() {
	wiimote **wiimotes;
	int found, connected;

	/*
	 *	Initialize an array of wiimote objects.
	 *
	 *	The parameter is the number of wiimotes I want to create.
	 */
	wiimotes = wiiuse_init(MAX_WIIMOTES);
	/*
	 *	Find wiimote devices
	 *
	 *	Now we need to find some wiimotes.
	 *	Give the function the wiimote array we created, and tell it there
	 *	are MAX_WIIMOTES wiimotes we are interested in.
	 *
	 *	Set the timeout to be 5 seconds.
	 *
	 *	This will return the number of actual wiimotes that are in discovery mode.
	 */
	found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
	if (!found)
	{
		printf("No wiimotes found, retrying...\n");
		if (connection()) return EXIT_FAILURE;
		found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
	}

	/*
	 *	Connect to the wiimotes
	 *
	 *	Now that we found some wiimotes, connect to them.
	 *	Give the function the wiimote array and the number
	 *	of wiimote devices we found.
	 *
	 *	This will return the number of established connections to the found wiimotes.
	 */
	connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
	if (connected)
	{
		printf("Connected to %i wiimotes (of %i found).\n", connected, found);
	}
	else
	{
		printf("Failed to connect to any wiimote.\n");
		return EXIT_FAILURE;
	}

	/*
	 *	Now set the LEDs and rumble for a second so it's easy
	 *	to tell which wiimotes are connected (just like the wii does).
	 */
	wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
	wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_2);
	wiiuse_set_leds(wiimotes[2], WIIMOTE_LED_3);
	wiiuse_set_leds(wiimotes[3], WIIMOTE_LED_4);
	wiiuse_rumble(wiimotes[0], 1);
	wiiuse_rumble(wiimotes[1], 1);

#ifndef WIIUSE_WIN32
	usleep(200000);
#else
	Sleep(200);
#endif
	wiiuse_rumble(wiimotes[0], 0);
	wiiuse_rumble(wiimotes[1], 0);

	/*
	 *	This is the main loop
	 *
	 *	wiiuse_poll() needs to be called with the wiimote array
	 *	and the number of wiimote structures in that array
	 *	(it doesn't matter if some of those wiimotes are not used
	 *	or are not connected).
	 *
	 *	This function will set the event flag for each wiimote
	 *	when the wiimote has things to report.
	 */

	char charPressed = ' ';
	while (any_wiimote_connected(wiimotes, MAX_WIIMOTES) && charPressed != '.')
	{

		if (_kbhit() != 0) {
			charPressed = _getch();
		}


		if (wiiuse_poll(wiimotes, MAX_WIIMOTES))
		{
			/*
			 *	This happens if something happened on any wiimote.
			 *	So go through each one and check if anything happened.
			 */
			int i = 0;
			for (; i < MAX_WIIMOTES; ++i)
			{
				switch (wiimotes[i]->event)
				{
				case WIIUSE_EVENT:
					/* a generic event occurred */
					handle_event(wiimotes[i]);
					break;

				case WIIUSE_STATUS:
					/* a status event occurred */
					handle_ctrl_status(wiimotes[i]);
					break;

				case WIIUSE_DISCONNECT:
				case WIIUSE_UNEXPECTED_DISCONNECT:
					/* the wiimote disconnected */
					handle_disconnect(wiimotes[i]);
					break;

				case WIIUSE_READ_DATA:
					/*
					 *	Data we requested to read was returned.
					 *	Take a look at wiimotes[i]->read_req
					 *	for the data.
					 */
					break;

				case WIIUSE_WII_BOARD_CTRL_INSERTED:
					printf("Balance board controller inserted.\n");
					break;

				case WIIUSE_WII_BOARD_CTRL_REMOVED:
					/* some expansion was removed */
					handle_ctrl_status(wiimotes[i]);
					printf("An expansion was removed.\n");
					break;

				default:
					break;
				}
			}
		}
	}

	/*
	 *	Disconnect the wiimotes
	 */
	wiiuse_cleanup(wiimotes, MAX_WIIMOTES);

	return EXIT_SUCCESS;
}

int connection() {
	HANDLE hRadios[256];
	int nRadios;
	int nPaired = 0;
	int loopErrors = 0;
	int numberOfDevices;
	char removePreviousWiiMotes;

	std::cout << "Enter the number of wiimotes you want to pair (Should be 1): ";
	std::cin >> numberOfDevices;
	std::cout << "Will continuse to pair until " << numberOfDevices << " are synced. \n";
	std::cout << "Should I remove all previously paired wiimotes (y/n) (You should do it, if your pad is already synced it can break some stuff): ";
	std::cin >> removePreviousWiiMotes;



	///////////////////////////////////////////////////////////////////////
	// Enumerate BT radios
	///////////////////////////////////////////////////////////////////////
	{
		HBLUETOOTH_RADIO_FIND hFindRadio;
		BLUETOOTH_FIND_RADIO_PARAMS radioParam;

		_tprintf(_T("Enumerating radios...\n"));

		radioParam.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);

		nRadios = 0;
		hFindRadio = BluetoothFindFirstRadio(&radioParam, &hRadios[nRadios++]);
		if (hFindRadio)
		{
			//while (BluetoothFindNextRadio(&radioParam, &hRadios[nRadios++]));
			while (BluetoothFindNextRadio(hFindRadio, &hRadios[nRadios++]));
			BluetoothFindRadioClose(hFindRadio);
		}
		else
		{
			ShowErrorCode(_T("Error enumerating radios"), GetLastError());
			system("pause");
			return (1);
		}
		nRadios--;
		_tprintf(_T("Found %d radios\n"), nRadios);
	}

	if (removePreviousWiiMotes == 'y')
	{
		std::cout << "\nPreparing to remove ALL previous wiimotes...\n";
		system("pause");
		loopThroughBTDevices(nRadios, nPaired, hRadios, removePreviousWiiMotes);
	}

	std::cout << "Successfully removed " << nPaired << " wiimotes!\n";
	std::cout << "\nI'm going to pair " << numberOfDevices << " wiimotes.\nYou will need to press the red button inside the battery compartment during this process.\n";
	system("pause");

	///////////////////////////////////////////////////////////////////////
	// Keep looping until we pair with a Wii device
	///////////////////////////////////////////////////////////////////////
	nPaired = 0, loopErrors = 0;
	while (nPaired < numberOfDevices && loopErrors < loopErrorMax)
	{
		std::cout << "Try " << loopErrors + 1 << " out of " << loopErrorMax << std::endl;
		loopThroughBTDevices(nRadios, nPaired, hRadios, 'n');
		loopErrors++;
	}
	if (loopErrors == loopErrorMax) {
		std::cout << "Critical error, exiting" << std::endl;
		system("pause");
		std::exit(EXIT_FAILURE);
	}

	///////////////////////////////////////////////////////////////////////
	// Clean up
	///////////////////////////////////////////////////////////////////////

	{
		int radio;

		for (radio = 0; radio < nRadios; radio++)
		{
			CloseHandle(hRadios[radio]);
		}
	}

	_tprintf(_T("=============================================\n"));
	_tprintf(_T("%d Wii devices paired\n"), nPaired);
	system("pause");

	return 0;
}

/*
 *	@brief Callback that handles an event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	This function is called automatically by the wiiuse library when an
 *	event occurs on the specified wiimote.
 */
void handle_event(struct wiimote_t *wm)
{
	printf("\n\n--- EVENT [id %i] ---\n", wm->unid);

	/* if a button is pressed, report it */
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_A))
	{
		printf("A pressed\n");
		firstRun = 0;
	}

	/* show events specific to supported expansions */
	/* wii balance board */
	struct wii_board_t *wb = (wii_board_t *)&wm->exp.wb;

	if (!firstRun)
	{
		frontLeftCal = wb->tl;
		frontRightCal = wb->tr;
		backLeftCal = wb->bl;
		backRightCal = wb->br;
		firstRun = 1;
	}
	frontLeft = wb->tl - frontLeftCal;
	frontRight = wb->tr - frontRightCal;
	backLeft = wb->bl - backLeftCal;
	backRight = wb->br - backRightCal;

	float total = frontLeft + frontRight + backLeft + backRight;
	float x = ((frontRight + backRight) / total) * 2 - 1;
	float y = ((frontLeft + frontRight) / total) * 2 - 1;
	printf("Weight: %f kg @ (%f, %f)\n", total, x, y);
	printf("Interpolated weight: frontLeft:%f  frontRight:%f  backLeft:%f  backRight:%f\n", frontLeft,
		frontRight, backLeft, backRight);
	printf("Raw: frontLeft:%d  frontRight:%d  backLeft:%d  backRight:%d\n", wb->rtl, wb->rtr, wb->rbl, wb->rbr);
}

/**
 *	@brief Callback that handles a read event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Pointer to the filled data block.
 *	@param len		Length in bytes of the data block.
 *
 *	This function is called automatically by the wiiuse library when
 *	the wiimote has returned the full data requested by a previous
 *	call to wiiuse_read_data().
 *
 *	You can read data on the wiimote, such as Mii data, if
 *	you know the offset address and the length.
 *
 *	The \a data pointer was specified on the call to wiiuse_read_data().
 *	At the time of this function being called, it is not safe to deallocate
 *	this buffer.
 */
void handle_read(struct wiimote_t *wm, byte *data, unsigned short len)
{
	int i = 0;

	printf("\n\n--- DATA READ [wiimote id %i] ---\n", wm->unid);
	printf("finished read of size %i\n", len);
	for (; i < len; ++i)
	{
		if (!(i % 16))
		{
			printf("\n");
		}
		printf("%x ", data[i]);
	}
	printf("\n\n");
}

/**
 *	@brief Callback that handles a controller status event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *	@param attachment		Is there an attachment? (1 for yes, 0 for no)
 *	@param speaker			Is the speaker enabled? (1 for yes, 0 for no)
 *	@param ir				Is the IR support enabled? (1 for yes, 0 for no)
 *	@param led				What LEDs are lit.
 *	@param battery_level	Battery level, between 0.0 (0%) and 1.0 (100%).
 *
 *	This occurs when either the controller status changed
 *	or the controller status was requested explicitly by
 *	wiiuse_status().
 *
 *	One reason the status can change is if the nunchuk was
 *	inserted or removed from the expansion port.
 */
void handle_ctrl_status(struct wiimote_t *wm)
{
	printf("\n\n--- CONTROLLER STATUS [wiimote id %i] ---\n", wm->unid);

	printf("attachment:      %i\n", wm->exp.type);
	printf("speaker:         %i\n", WIIUSE_USING_SPEAKER(wm));
	printf("ir:              %i\n", WIIUSE_USING_IR(wm));
	printf("leds:            %i %i %i %i\n", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2),
		WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
	printf("battery:         %f %%\n", wm->battery_level);
}

/**
 *	@brief Callback that handles a disconnection event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *
 *	This can happen if the POWER button is pressed, or
 *	if the connection is interrupted.
 */
void handle_disconnect(wiimote *wm) { printf("\n\n--- DISCONNECTED [wiimote id %i] ---\n", wm->unid); }

short any_wiimote_connected(wiimote **wm, int wiimotes)
{
	int i;
	if (!wm)
	{
		return 0;
	}

	for (i = 0; i < wiimotes; i++)
	{
		if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i]))
		{
			return 1;
		}
	}

	return 0;
}

/* Some more functions used to connect to the board*/
DWORD ShowErrorCode(const wchar_t  *msg, DWORD dw)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	_tprintf(_T("%s: %s"), msg, (wchar_t*)lpMsgBuf);

	LocalFree(lpMsgBuf);

	return dw;
}
_TCHAR * FormatBTAddress(BLUETOOTH_ADDRESS address)
{
	static _TCHAR ret[20];
	_stprintf_s(ret, _T("%02x:%02x:%02x:%02x:%02x:%02x"),
		address.rgBytes[5],
		address.rgBytes[4],
		address.rgBytes[3],
		address.rgBytes[2],
		address.rgBytes[1],
		address.rgBytes[0]
	);
	return ret;
}
int loopThroughBTDevices(int nRadios, int &nPaired, HANDLE* hRadios, char remove) {

	int radio;
	TCHAR devName[256];

	for (radio = 0; radio < nRadios; radio++)
	{
		BLUETOOTH_RADIO_INFO radioInfo;
		HBLUETOOTH_DEVICE_FIND hFind;
		BLUETOOTH_DEVICE_INFO btdi;
		BLUETOOTH_DEVICE_SEARCH_PARAMS srch;

		radioInfo.dwSize = sizeof(radioInfo);
		btdi.dwSize = sizeof(btdi);
		srch.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);

		//ShowErrorCode(_T("BluetoothGetRadioInfo"), BluetoothGetRadioInfo(hRadios[radio], &radioInfo));

		/*
		_tprintf(_T("Radio %d: %ls %s\n"),
			radio,
			radioInfo.szName,
			FormatBTAddress(radioInfo.address)
		);
		*/

		BluetoothGetRadioInfo(hRadios[radio], &radioInfo);

		srch.fReturnAuthenticated = TRUE;
		srch.fReturnRemembered = TRUE;
		srch.fReturnConnected = TRUE;
		srch.fReturnUnknown = TRUE;
		srch.fIssueInquiry = TRUE;
		srch.cTimeoutMultiplier = 2;
		srch.hRadio = hRadios[radio];

		_tprintf(_T("Scanning for devices on bluetooth radio %d - %ls with address %s...\n"), radio, radioInfo.szName, FormatBTAddress(radioInfo.address));

		hFind = BluetoothFindFirstDevice(&srch, &btdi);

		if (hFind == NULL)
		{
			if (GetLastError() == ERROR_NO_MORE_ITEMS)
			{
				_tprintf(_T("No bluetooth devices found.\n"));
			}
			else
			{
				ShowErrorCode(_T("Error enumerating devices"), GetLastError());
				system("pause");
				return (1);
			}
		}
		else
		{
			do
			{
				//_tprintf(_T("Found: %s\n"), btdi.szName);

				if (!wcscmp(btdi.szName, L"Nintendo RVL-WBC-01") || !wcscmp(btdi.szName, L"Nintendo RVL-CNT-01"))
				{
					WCHAR pass[6];
					DWORD pcServices = 16;
					GUID guids[16];
					int error = 0;

					_sntprintf_s(devName, 256, _T("%ls %s"), btdi.szName, FormatBTAddress(btdi.Address));
					_tprintf(_T("Found: %s\n"), devName);

					if (btdi.fRemembered)
					{
						if (remove == 'y')
						{
							// Make Windows forget pairing
							_tprintf(_T("Removing: %s\n"), devName);
							ShowErrorCode(_T("BluetoothRemoveDevice"), BluetoothRemoveDevice(&btdi.Address));
							nPaired++;
						}
						continue;
					}

					// MAC address is passphrase
					pass[0] = radioInfo.address.rgBytes[0];
					pass[1] = radioInfo.address.rgBytes[1];
					pass[2] = radioInfo.address.rgBytes[2];
					pass[3] = radioInfo.address.rgBytes[3];
					pass[4] = radioInfo.address.rgBytes[4];
					pass[5] = radioInfo.address.rgBytes[5];

					if (!error)
					{
						// Pair with Wii device
						#pragma warning(disable: 4995)
						if (ShowErrorCode(_T("BluetoothAuthenticateDevice"), BluetoothAuthenticateDevice(NULL, hRadios[radio], &btdi, pass, 6)) != ERROR_SUCCESS)
						#pragma warning(default: 4995)
							error = 1;
					}

					if (!error)
					{
						// If this is not done, the Wii device will not remember the pairing
						if (ShowErrorCode(_T("BluetoothEnumerateInstalledServices"), BluetoothEnumerateInstalledServices(hRadios[radio], &btdi, &pcServices, guids)) != ERROR_SUCCESS)
							error = 1;
					}

					if (!error)
					{
						// Activate service
						if (ShowErrorCode(_T("BluetoothSetServiceState"), BluetoothSetServiceState(hRadios[radio], &btdi, &HumanInterfaceDeviceServiceClass_UUID, BLUETOOTH_SERVICE_ENABLE)) != ERROR_SUCCESS)
							error = 1;
					}

					if (!error)
					{
						nPaired++;
					}
					else {
						std::cout << "Critical error, exiting" << std::endl;
						system("pause");
						std::exit(EXIT_FAILURE);
					}
				} // if (!wcscmp(btdi.szName, L"Nintendo RVL-WBC-01") || !wcscmp(btdi.szName, L"Nintendo RVL-CNT-01"))
			} while (BluetoothFindNextDevice(hFind, &btdi));
			BluetoothFindDeviceClose(hFind);

		} // if (hFind == NULL)
	} // for (radio = 0; radio < nRadios; radio++)

	Sleep(1000);
	return EXIT_SUCCESS;
}