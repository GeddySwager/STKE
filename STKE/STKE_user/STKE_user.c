/*
This program is used to interact with its corresponding kernel driver.
*/

#include "STKE_user.h"

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

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
        if (!(isdigit(selection[0])))
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
                    listDrivers();
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