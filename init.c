/*____________________________________________________________________________
**
** File:          init.c
**
** Description:   Initialization of the input/output interface
**____________________________________________________________________________
*/

#include <stdio.h>
#ifndef  MSDOS
   #ifdef sco
      #include <sys/console.h>
   #endif
   #include <unistd.h>
#endif
#include "inter.h"
#include "check.h"

void  DosInit(void);
void  CursesInit(void);
void  ConsoleInit(void);

void  OpenInterface(void)
{
   _I_Screen=NULL;
   _I_ColorMap=NULL;
   _I_PairMap=NULL;
   _I_ColorInfo=NULL;
   _I_PairInfo=NULL;
   I_CursorX=I_CursorY=0;
   I_CurType=C_NORMAL;
   AddMemoryErrorHandler(CloseInterface,200);
#ifdef MSDOS
   DosInit();
#else
#ifdef sco
   if(ioctl(0,CONS_GET,0)==-1)
      CursesInit();
   else
      SCOConsoleInit();
#else /* defined(sco) */
   CursesInit();
#endif
#endif /* MSDOS */
   TimerInit();
   _I_OldMouseX=I_MouseX;
   _I_OldMouseY=I_MouseY;
   _I_OldMouseOffs=_I_MouseOffs=I_MouseY*I_ScreenWidth+I_MouseX;
   I_EventMask=KEYBOARD_EVENTS;
   _I_HaveReadKey=0;
}
