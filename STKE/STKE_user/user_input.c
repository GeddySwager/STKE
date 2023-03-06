#include "user_input.h"

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
    UINT count = 0;
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