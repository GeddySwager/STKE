/*
This program will eventually interact with the kernel driver, right now it just lists drivers.
*/

#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <iostream>

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

#define ARRAY_SIZE 1024

int Error(const char* msg)
{
    printf("%s: error=%d\n", msg, GetLastError());
    return 1;
}

// code from: https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-device-drivers-in-the-system
int ListDrivers()
{
    LPVOID drivers[ARRAY_SIZE];
    DWORD cbNeeded;
    int cDrivers, i;

    if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
    {
        TCHAR szDriver[ARRAY_SIZE];

        cDrivers = cbNeeded / sizeof(drivers[0]);

        _tprintf(TEXT("There are %d drivers:\n"), cDrivers);
        for (i = 0; i < cDrivers; i++)
        {
            if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
            {
                _tprintf(TEXT("%d: %s\n"), i + 1, szDriver);
            }
        }
    }
    else
    {
        _tprintf(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), cbNeeded / sizeof(LPVOID));
        return 1;
    }

    return 0;
}


int main(void)
{
    int option;
    while (TRUE)
    {
        printf("\nSTKE available functions:\n");
        printf("1. List all installed or running drivers\n");
        printf("2. Stop program\n");
        std::cin >> option;

        if (option > 1)
        {
            printf("\nPlease select from the available options.\n");
        }
        else
        {
            switch (option)
            {
            case 0:
                return 0;
                break;
            
            case 1:
                ListDrivers();
                break;
            }

        }
    }
}