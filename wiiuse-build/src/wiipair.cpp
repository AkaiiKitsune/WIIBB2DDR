#include "stdafx.h"

#include <windows.h>
#include <bthsdpdef.h>
#include <bthdef.h>
#include <BluetoothAPIs.h>
#include <strsafe.h>
#include <iostream>

using namespace std;
#pragma comment(lib, "Bthprops.lib")

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

	_tprintf_s(_T("%s: %s"), msg, lpMsgBuf);

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
					BOOL error = FALSE;

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
						if (ShowErrorCode(_T("BluetoothAuthenticateDevice"), BluetoothAuthenticateDevice(NULL, hRadios[radio], &btdi, pass, 6)) != ERROR_SUCCESS)
							error = TRUE;
					}

					if (!error)
					{
						// If this is not done, the Wii device will not remember the pairing
						if (ShowErrorCode(_T("BluetoothEnumerateInstalledServices"), BluetoothEnumerateInstalledServices(hRadios[radio], &btdi, &pcServices, guids)) != ERROR_SUCCESS)
							error = TRUE;
					}

					if (!error)
					{
						// Activate service
						if (ShowErrorCode(_T("BluetoothSetServiceState"), BluetoothSetServiceState(hRadios[radio], &btdi, &HumanInterfaceDeviceServiceClass_UUID, BLUETOOTH_SERVICE_ENABLE)) != ERROR_SUCCESS)
							error = TRUE;
					}

					if (!error)
					{
						nPaired++;
					}
				} // if (!wcscmp(btdi.szName, L"Nintendo RVL-WBC-01") || !wcscmp(btdi.szName, L"Nintendo RVL-CNT-01"))
			} while (BluetoothFindNextDevice(hFind, &btdi));
			BluetoothFindDeviceClose(hFind);

		} // if (hFind == NULL)
	} // for (radio = 0; radio < nRadios; radio++)

	Sleep(1000);
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hRadios[256];
	int nRadios;
	int nPaired = 0;
	int numberOfDevices;
	char removePreviousWiiMotes;

	cout << "Enter the number of wiimotes you want to pair (Should be 1, as you only need the wii motion pad) : ";
	cin >> numberOfDevices;
	cout << "Will continuse to pair until " << numberOfDevices << " are synced. \n";
	cout << "Should I remove all previously paired wiimotes (y/n) (default : n) : ";
	cin >> removePreviousWiiMotes;
	


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
		cout << "\nPreparing to remove ALL previous wiimotes...\n";
		system("pause");
		loopThroughBTDevices(nRadios, nPaired, hRadios, removePreviousWiiMotes);
	}

	cout << "Successfully removed " << nPaired << " wiimotes!\n";
	cout << "\nI'm going to pair " << numberOfDevices << " wiimotes.\nYou will need to press the red button inside the battery compartment during this process.\n";
	system("pause");

	///////////////////////////////////////////////////////////////////////
	// Keep looping until we pair with a Wii device
	///////////////////////////////////////////////////////////////////////
	nPaired = 0;
	while (nPaired < numberOfDevices)
	{
		loopThroughBTDevices(nRadios, nPaired, hRadios, 'n');
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

	_tprintf_s(_T("=============================================\n"), nPaired);
	_tprintf_s(_T("%d Wii devices paired\n"), nPaired);
	system("pause");

	return 0;
}