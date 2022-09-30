#include "include/game.h"
#include <stdio.h>

int main()
{
    for(int i=0;i<52;i++)
    {
        if(!(i%13)) printf("\n");
        print_card(i);
        printf(" ");
    }

    return 0;
}