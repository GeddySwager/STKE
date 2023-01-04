/*
This program is used to interact with its corresponding kernel driver.
*/

#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <Shlwapi.h>

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

#define ARRAY_SIZE 1024
#define IOCTL_STKE_LOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

int Error(LPCTSTR lpszFunction)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and clean up

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen(lpszFunction) + 40) * sizeof(TCHAR));
    if (lpDisplayBuf != NULL)
    {
        StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%ws failed with error %d: %ws"),
            lpszFunction, dw, (LPTSTR)lpMsgBuf);
        printf("%ws", (LPTSTR)lpDisplayBuf);
    }
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    return 1;
}


BOOL GetInput(PWCHAR buf, DWORD charsToRead)
{
    BOOL status = TRUE;
    DWORD charCount = 0, ch, dwConsoleMode;
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(console, &dwConsoleMode);
    printf("%x\n", dwConsoleMode);
    printf("%x\n", (dwConsoleMode & ~(ENABLE_LINE_INPUT)));
    //SetConsoleMode(console, (dwConsoleMode & ~(ENABLE_LINE_INPUT)));
    //Error(L"SetConsoleMode");
    printf("%x\n", dwConsoleMode);
    do {
        status = ReadConsoleW(console, &buf[charCount], 1, &ch, NULL);
        charCount++;
    } while (((charCount + 1) < charsToRead) && (buf[charCount] != '\r'));
    /*if (charCount > charsToRead)
    {
        printf("You entered too many characters.\n");
        CloseHandle(console);
        return 0;
    }*/

    SetConsoleMode(console, dwConsoleMode);

    return status;
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
        return (wcscmp(string, L".sys"));
    }
    return 1;
}

int loadDriver()
{
    printf("Type out the full path to the driver, including driver name (example \"C:\\Users\\vagrant\\desktop\\driver.sys\")\n\n");
    printf("Windows accepts a maximum path of 260 characters.\n\n");
    WCHAR regPath[MAX_PATH + 2] = { 0 };
    

    while (TRUE)
    {
        if (!GetInput(regPath, _countof(regPath)))
        {
            printf("Somehow broke ReadConsole function.\n");
        }
        else if (!wcscmp(regPath, L"q"))
        {
            printf("Exiting.\n");
            return 1;
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
                break;
            }
            else
            {
                printf("Invalid input, file either does not exist or you have input a directory instead of a file.\n");
            }
        }
    }
    printf("Driver found! Please provide a name for your driver (limit 8 characters).\n");

    WCHAR driverName[10] = { 0 };

    while (TRUE)
    {
        if (!GetInput(driverName, _countof(driverName)))
        {
            printf("Somehow broke ReadConsole function.\n");
        }
        else if (!wcscmp(driverName, L"q"))
        {
            printf("Exiting.\n");
            return 1;
        }
        else
        {
            break;
        }
    }

    HKEY key;
    DWORD keyOptions = REG_OPTION_VOLATILE;
    WCHAR subKey[] = L"SYSTEM\\CurrentControlSet\\Services\\";
    WCHAR keyValue1[] = L"ImagePath";
    WCHAR keyValue2[] = L"Type";

    WCHAR keyData[MAX_PATH + 4];
    ZeroMemory(keyData, sizeof(keyData));
    if (FAILED(StringCchCopyW(keyData, (MAX_PATH + 4), L"\\??\\")))
    {
        return Error(L"String copy failed.\n");
    }
    if (FAILED(StringCchCatW(keyData, (MAX_PATH + 4), regPath)))
    {
        return Error(L"String concatenation failed.\n");
    }
    printf("Test: %ws\n", keyData);



    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, keyOptions, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &key, NULL);

    if (status != ERROR_SUCCESS)
    {
        return Error(L"Failed to create registry key.\n");
    }

    //status = RegSetKeyValueW(key, NULL, )

    HANDLE hDevice = CreateFile(L"\\\\.\\STKE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return Error(L"Failed to open device.\n");
    }



    return 0;
}


int main(void)
{
    LONG option;
    WCHAR selection[3];
    while (TRUE)
    {
        printf("\nSTKE available functions:\n");
        printf("1. List all installed or running drivers\n");
        printf("2. Load driver\n");
        printf("0. Stop program\n\n");
        if(!(GetInput(selection, 3)))
        {
            printf("Somehow broke ReadConsole function.\n");
        }
        else if (!(isdigit(selection[0])))
        {
            printf("You did not enter a number.\n");
        }
        else
        {
            option = wcstol(selection, NULL, 0);
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
    }

    return 0;
}