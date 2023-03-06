#include "loading.h"

#define IOCTL_STKE_LOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

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
    WCHAR subKey[44];
    ZeroMemory(subKey, sizeof(subKey));
    if (FAILED(StringCchCopyW(subKey, 44, L"SYSTEM\\CurrentControlSet\\Services\\")))
    {
        return Error(L"String copy");
    }
    if (FAILED(StringCchCatW(subKey, 44, driverName)))
    {
        return Error(L"Subkey and driver name concatenation");
    }
    printf("Registry subkey will be set to: %ws\n", subKey);

    WCHAR keyValue1[] = L"ImagePath";
    WCHAR keyValue2[] = L"Type";
    DWORD keyValue2Data = 1;

    WCHAR keyData[MAX_PATH + 5];
    ZeroMemory(keyData, sizeof(keyData));
    if (FAILED(StringCchCopyW(keyData, (MAX_PATH + 5), L"\\??\\")))
    {
        return Error(L"String copy");
    }
    if (FAILED(StringCchCatW(keyData, (MAX_PATH + 5), regPath)))
    {
        return Error(L"Global root and regPath concatenation");
    }
    printf("Registry key \"ImagePath\" will be set to: %ws\n", keyData);



    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, keyOptions, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &key, NULL);

    if (status != ERROR_SUCCESS)
    {
        printf("RegCreateKeyExW returned error code: %d \n Make sure to run the program as admin.\n", status);
        return 1;
    }

    status = RegSetKeyValueW(key, NULL, keyValue1, REG_SZ, keyData, (DWORD)((wcslen(keyData) + 1) * sizeof(WCHAR)));
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

    WCHAR full_reg_path[63];
    ZeroMemory(full_reg_path, sizeof(full_reg_path));
    StringCchCopyW(full_reg_path, 63, L"\\registry\\machine\\");
    StringCchCatW(full_reg_path, 63, subKey);

    HANDLE hDevice = CreateFile(L"\\\\.\\STKE", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return Error(L"CreateFile");
    }

    DWORD bytesReturned;
    BOOL success = DeviceIoControl(hDevice, IOCTL_STKE_LOAD_DRIVER, full_reg_path, (DWORD)(wcslen(full_reg_path) * sizeof(WCHAR)), NULL, 0, &bytesReturned, NULL);

    if (!success)
    {
        return Error(L"DeviceIoControl");
    }

    printf("Driver should now be successfully loaded.");

    return 0;
}