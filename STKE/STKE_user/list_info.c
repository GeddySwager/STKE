#include "list_info.h"

#define ARRAY_SIZE 1024

// code from: https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-device-drivers-in-the-system
int listDrivers()
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
        _tprintf(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), (int)(cbNeeded / sizeof(LPVOID)));
        return 1;
    }

    return 0;
}