/*
This program is used to interact with its corresponding kernel driver.
*/

#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <stdio.h>
#include <Shlwapi.h>

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
        _tprintf(TEXT("EnumDeviceDrivers failed; array size needed is %d\n"), (int) (cbNeeded / sizeof(LPVOID)));
        return 1;
    }

    return 0;
}

int endsWithSys(WCHAR* string)
{
    string = StrRChrW(string, NULL, L'.');
    if (string != NULL)
    {
        return(wcscmp(string, L".sys"));
    }
    return(-1);
}

int loadDriver()
{
    printf("Type out the full path to the driver, including driver name (example \"C:\\Users\\vagrant\\desktop\\driver.sys\")\n");
    WCHAR regPath[MAX_PATH] = { 0 };

    while (TRUE)
    {
        int status = wscanf_s(L"%s", regPath, (MAX_PATH - 1));
        if (!status || status == EOF)
        {
            printf("Windows accepts a maximum path of 260 characters.\n");
        }
        else if (!wcscmp(regPath, L"q"))
        {
            printf("Exiting.\n");
            return -1;
        }
        else if (endsWithSys(regPath))
        {
            printf("Your path either did not end with \'.sys\' or was null.\n");
        }
        else
        {
            DWORD dwAttrib = GetFileAttributes(regPath);
            if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (regPath)
                break;
            }
            else
            {
                printf("Invalid input, file either does not exist or you have input a directory instead of a file.\n");
            }
        }
    }
    printf("Driver found! Returning to start menu as feature is not finished.\n");
    return 0;
}


int main(void)
{
    int option;
    while (TRUE)
    {
        printf("\nSTKE available functions:\n");
        printf("1. List all installed or running drivers\n");
        printf("2. Load driver\n");
        printf("0. Stop program\n");
        wscanf_s(L"%d", &option);

        if (option > 2)
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

            case 2:
                loadDriver();
                break;
            }

        }
    }

    return 0;
}