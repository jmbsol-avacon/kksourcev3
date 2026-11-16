/**
*   MENU.C      -
*
*
*   Description -
*
*
*
*
*   (c) Copyright 1994 by Brian Weed for Automation Solutions, Inc.
*   All rights reserved.
*
*
*   Revision History:
*
*   Version     Date        Author      Comments
*   -------     --------    ------      --------------------------------
*   3.00a       09-07-93     BCW        Revision 3
*
**/
#include <string.h>
#include <conio.h>
#include "db_defs.h"
#include "vmisc.h"

/*****************************************************************************
menu - generic menu routine.
    prompt: array of prompt strings.
    cols:   columns of menu (1 or 2)
    col1x:  x coordinate of column 1
    col2x:  x coordinate of column 2
    start:  initial hilighted menu option
*****************************************************************************/
INT menu(CHAR **prompt, INT cols, INT col1x, INT col2x, INT start, INT helptype)
{
    INT i, choice, y, endi, xpos[12], ypos[12], lastchoice, user, max, inchar;

    /* paint screen */
    mainscreen();
    SetCursor(CUR_OFF);

    /* print title */
    printxy(37 - strlen(prompt[0])/2, 7, "%s", prompt[0]);

    /* find how many menu options */
    for (max=0; prompt[max+1] != NULL; max++)
        ;
    /* 
    * don't display the last menu on purpose
    */
    max--;

    /*
    * for each menu item, print prompt
    */
    endi = 1 + (max - 1)/cols;
    for (i=1,choice=endi+1; i<=endi; i++, choice++) {
        xpos[i] = col1x;
        ypos[i] = 2*i+7;
        printxy(col1x, ypos[i], "%d) %s", i, prompt[i]);

        if (cols == 2 && choice <= max) {
            xpos[choice] = col2x;
            ypos[choice] = 2*i+7;
            printxy(col2x, ypos[choice], "%d) %s", choice, prompt[choice]);
        }
    }

    /*
    * display the last menu item 
    */
    if(cols == 1)
    {
        xpos[i] = col1x;
        ypos[i] = 2*i+7;
        printxy(col1x, ypos[i], "%d) %s", i, prompt[max+1]);
    }
    else if((max/2)*2 < max) /* even menu items */
    {    
        xpos[choice-1] = col2x;
        ypos[choice-1] = 2*(i-1)+7;
        printxy(col2x, ypos[choice-1], "%d) %s", choice-1, prompt[max+1]);
    }
    else
    {    
        xpos[choice] = 37 - strlen(prompt[max+1])/2;
        ypos[choice] = 2*i+7;
        printxy(xpos[choice], ypos[choice], "%d) %s", choice, prompt[max+1]);
    }

    /*
    * restore max 
    */
    max++;
    if(cols == 1)
        endi++;

    y = 2 * endi + 11;
    if (y >= 25)
        y = 24;

    printxy(25, y, "PLEASE ENTER YOUR SELECTION");
    lastchoice = start;

    do {
        textattr(REVERSE);
        printxy(xpos[lastchoice], ypos[lastchoice], "%d) %s ", lastchoice, prompt[lastchoice]);
        textattr(NORMAL);
        user = 0;
        choice = nextkey();
        inchar = choice&0xff;
        printxy(xpos[lastchoice], ypos[lastchoice], "%d) %s ", lastchoice, prompt[lastchoice]);

        if (!inchar) {

            switch (choice) {

                case KEY_UP:
                    if (lastchoice == 1)
                        break;
                    lastchoice--;
                    break;

                case KEY_DOWN:
                    if (lastchoice == max)
                        break;
                    lastchoice++;
                    break;

                case KEY_RIGHT:
                    if(cols == 1)
                    {
                        lastchoice++;
                        if(lastchoice > max)
                            lastchoice = 1;
                        break;
                    }
                    if (lastchoice > endi)
                        break;
                    lastchoice += endi;

                    if (lastchoice > max)
                        lastchoice = max;
                    break;

                case KEY_LEFT:
                    if(cols == 1)
                    {
                        lastchoice--;
                        if(lastchoice < 1)
                            lastchoice = max;
                        break;
                    }
                    if (lastchoice <= endi)
                        break;
                    if(lastchoice == max && (max/2)*2 < max)
                        break;
                    lastchoice -= endi;
                    break;

                case KEY_F1:
                    helpmessage(helptype);
                    break;

                default:
                    return(choice);
            }
        } else {

            switch (inchar) {

                case KEY_ESC1:
                    return(KEY_F10);

                case KEY_CR:
                    return(lastchoice);

                default:
                    /*
                    * flash the menu item that was selected
                    */
                    inchar -= 48;
                    if (inchar >= 1 && inchar <= max) {
                        textattr(REVERSE);
                        printxy(xpos[inchar], ypos[inchar], "%d) %s ", inchar, prompt[inchar]);
                        textattr(NORMAL);
                    }
                    return(inchar);
            }
        }

    } while (user>max || !user);

    return(user);
}
