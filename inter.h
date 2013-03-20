/*____________________________________________________________________________
**
** File:          inter.h
**
** Description:   Header file for input/output interface
**____________________________________________________________________________
*/

#include "keys.h"

#ifndef  TRUE
#define  TRUE  1
#define  FALSE 0
#endif

typedef  unsigned short Char;
typedef  unsigned short Color;
typedef  unsigned       Key;

#define  MODIFIED    1
#define  BLINK       2
#define  BOLD        4
#define  DIM         8
#define  REVERSE     16
#define  UNDERLINE   32
#define  COLOR       64
#define  GRAPHIC     128

#define  READONLY    256

#define  COLORALLOCATED  512

typedef  struct   rgb
{
   unsigned char  r,g,b;
}
   Rgb;

typedef  struct   cell
{
   unsigned short flags;
   Char     ch;
   Rgb      foreground;
   Rgb      background;
}
   Cell;

typedef  struct   screencell
{
   unsigned short flags;
   Char  ch;
   Color foreground;
   Color background;
   unsigned pair;
   Rgb   foregroundrgb;
   Rgb   backgroundrgb;
}
   ScreenCell;

#define  CONSOLE           0x0001
#define  HAS_MOUSE         0x0002
#define  SHOW_MOUSE        0x0004
#define  CAN_CHANGE_COLOR  0x0020
#define  HAS_COLORS        0x0040
#define  HAS_PAIRS         0x0080
#define  SUSPENDED         0x0100

#define  LEFT_BUTTON       0x01
#define  MIDDLE_BUTTON     0x02
#define  CENTER_BUTTON     MIDDLE_BUTTON
#define  RIGHT_BUTTON      0x04

#define  S_TERM   0
#define  S_VGA    1
#define  S_EGA    2
#define  S_CGA    3
#define  S_MDA    4

#define  C_INVISIBLE 0
#define  C_NORMAL    1
#define  C_LARGE     2

#define  KEYBOARD_EVENTS   1
#define  MOUSE_EVENTS      2
#define  OTHER_EVENTS      4

typedef  struct   rgbstate
{
   unsigned short flags;
   unsigned refs;
}
   RgbState;

typedef  struct
{
   unsigned foreground;
   unsigned background;
}
   Pair;

typedef  struct   pairstate
{
   unsigned short flags;
   unsigned refs;
}
   PairState;

extern   unsigned I_ScreenWidth;      /* dimensions of the screen */
extern   unsigned I_ScreenHeight;
extern   unsigned I_ScreenSize;
extern   unsigned I_ScreenType;       /* type of the screen */
extern   unsigned I_ScreenFlags;      /* flags of the screen */

extern   ScreenCell  *_I_Screen;       /* local screen contents */

extern   void  OpenInterface(void);
extern   void  CloseInterface(void);
extern   void  SuspendInterface(void);
extern   void  ResumeInterface(void);

extern   void  SetScreenCell(unsigned x,unsigned y,Cell *c);
extern   void  GetScreenCell(unsigned x,unsigned y,Cell *c);
extern   void  Sync(void);
extern   void  RedrawScreen(void);

extern   void  MoveCursor(unsigned x,unsigned y);
extern   void  CursorType(int type);
extern   void  ShowMouse(int how);
extern   void  Bell(void);
extern   unsigned long  Timer(void);
extern   void  TimerInit(void);

extern   unsigned ReadKey(void);
extern   unsigned WaitKey(int timeout);
extern   int   KeyPressed(void);
extern   void  SetEventMask(unsigned newmask);

extern   void  (*_CloseInterface)(void);
extern   void  (*_DefineColor)(unsigned color,Rgb val);
extern   void  (*_DefinePair)(unsigned pair,unsigned foreground,unsigned background);
extern   void  (*_UpdateCell)(unsigned x,unsigned y,unsigned offs,ScreenCell *cell);
extern   int   (*_KeyPressed)(void);
extern   unsigned (*_ReadKey)(int timeout);
extern   void  (*_SetCursor)(unsigned x,unsigned y,int type);
extern   void  (*_Sync)(void);
extern   void  (*_Suspend)(void);
extern   void  (*_Resume)(void);
extern   void  (*_Redraw)(void);
extern   void  (*_Bell)(void);

extern   Rgb         *_I_ColorMap;        /* current color values */
extern   RgbState    *_I_ColorInfo;
extern   unsigned    I_ColorsNum;
extern   Pair        *_I_PairMap;
extern   PairState   *_I_PairInfo;
extern   unsigned    I_PairsNum;

extern   unsigned I_CursorX,I_CursorY;
extern   int      I_CurType;
extern   int      I_MouseX,I_MouseY;
extern   unsigned I_LastButton;
extern   unsigned I_Buttons;
extern   long     I_LastTime;
extern   Key      I_LastKey;
extern   unsigned I_EventMask;

extern   int      _I_HaveReadKey;
extern   int      _I_OldMouseX,_I_OldMouseY;
extern   unsigned _I_MouseOffs,_I_OldMouseOffs;

#define  HasColors()       (I_ScreenFlags&HAS_COLORS)
#define  CanChangeColor()  (I_ScreenFlags&CAN_CHANGE_COLOR)

extern   unsigned K_F(int);
extern   unsigned K_Shift_F(int);
extern   unsigned K_Ctrl_F(int);
extern   unsigned K_Alt_F(int);
