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


// This function can be added after any other function call to determine success.
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

/*
    Needed some way to get user input and decided to replicate a getchar() loop but
    with winapi calls for practice. GetInput reads one character at a time and writes
    to the buffer until it's full or the user hits enter. 
*/
BOOL GetInput(WCHAR buf[], DWORD charsToRead)
{
    // Code mostly taken from: https://stackoverflow.com/questions/69911010/readfile-only-returns-after-newline
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    // Get the current console mode
    DWORD mode;
    GetConsoleMode(hInput, &mode);

    // Save the current mode, so we can restore it later
    DWORD original_mode = mode;

    // Disable the line input mode
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    // And set the new mode
    SetConsoleMode(hInput, mode);

    // Then read and write input...
    INT count = 0;
    while (TRUE)
    {
        WCHAR buffer[1];
        DWORD nread = 0;

        ReadConsoleW(hInput, buffer, 1, &nread, NULL);
        WriteConsoleW(hOutput, buffer, nread, NULL, NULL);

        // Handle backspace or else user cannot make any mistakes while typing
        if (buffer[0] == '\b')
        {
            if (count > 0)
            {
                count--;
            }
            WriteConsoleW(hOutput, L" \b", 2, NULL, NULL);
        }
        // Allow some way to exit the program, ensure we don't read past buf size
        else if (buffer[0] == '\r' || buffer[0] == '\n')
        {
            /* 
             No need to zero out the remainder of buf memory in case of failed
             previous attempts as long as we have a null terminator. Surely this
             can't go wrong in the future! 
             */
            buf[count] = L'\0';
            break;
        }
        else
        {
            buf[count] = buffer[0];
            count++;
        }

        // There's probably a prettier way to do this, but it's good enough for now
        if ((count + 1) >= charsToRead)
        {
            buf[count] = L'\0';
            break;
        }
    }

    // Restore the original console mode
    SetConsoleMode(hInput, original_mode);

    // Without this newline the next thing to print to console will overwrite the user entry on the screen
    printf("\n");

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
        return (wcscmp(string, L".sys"));
    }
    return 1;
}

int loadDriver()
{
    printf("Type out the full path to the driver, including driver name (example \"C:\\Users\\vagrant\\desktop\\driver.sys\")\n\n");
    printf("Windows accepts a maximum path of 260 characters.\n\n");
    WCHAR regPath[MAX_PATH + 1] = { 0 };
    

    while (TRUE)
    {
        if (!GetInput(regPath, _countof(regPath)))
        {
            printf("Somehow broke GetInput function.\n");
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
            DWORD dwAttrib = GetFileAttributesW(regPath);
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
    printf("Driver file found! Please provide a name for your driver (limit 8 characters).\n");

    WCHAR driverName[9] = { 0 };

    while (TRUE)
    {
        if (!GetInput(driverName, _countof(driverName)))
        {
            printf("Somehow broke GetInput function.\n");
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
    WCHAR subKey[43];
    ZeroMemory(subKey, sizeof(subKey));
    if (FAILED(StringCchCopyW(subKey, 43, L"SYSTEM\\CurrentControlSet\\Services\\")))
    {
        return Error(L"String copy failed.\n");
    }
    if (FAILED(StringCchCatW(subKey, 43, driverName)))
    {
        return Error(L"Subkey and driver name concatenation failed.\n");
    }
    printf("Registry subkey will be set to: %ws\n", subKey);

    WCHAR keyValue1[] = L"ImagePath";
    WCHAR keyValue2[] = L"Type";
    DWORD keyValue2Data = 1;

    WCHAR keyData[MAX_PATH + 5];
    ZeroMemory(keyData, sizeof(keyData));
    if (FAILED(StringCchCopyW(keyData, (MAX_PATH + 5), L"\\??\\")))
    {
        return Error(L"String copy failed.\n");
    }
    if (FAILED(StringCchCatW(keyData, (MAX_PATH + 5), regPath)))
    {
        return Error(L"Global root and regPath concatenation failed.\n");
    }
    printf("Registry key \"ImagePath\" will be set to: %ws\n", keyData);



    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, keyOptions, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &key, NULL);
    
    if (status != ERROR_SUCCESS)
    {
        printf("RegCreateKeyExW returned error code: %d \n Make sure to run the program as admin.\n", status);
        return 1;
    }

    status = RegSetKeyValueW(key, NULL, keyValue1, REG_SZ, keyData, sizeof(keyData));
    if (status != ERROR_SUCCESS)
    {
        printf("RegSetKeyValueW for \"ImagePath\" returned error code: %d \n", status);
        return 1;
    }

    status = RegSetKeyValueW(key, NULL, keyValue2, REG_DWORD, &keyValue2Data, sizeof(DWORD));
    if (status != ERROR_SUCCESS)
    {
        printf("RegSetKeyValueW for \"Type\" returned error code: %d \n", status);
        return 1;
    }

    RegCloseKey(key);


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
    WCHAR selection[2];
    while (TRUE)
    {
        printf("\nSTKE available functions:\n");
        printf("1. List all installed or running drivers\n");
        printf("2. Load driver\n");
        printf("0. Stop program\n\n");
        if(!(GetInput(selection, _countof(selection))))
        {
            printf("Somehow broke GetInput function.\n");
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