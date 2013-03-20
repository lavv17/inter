/*____________________________________________________________________________
**
** File: inter.c
**____________________________________________________________________________
*/

#include <stdio.h>
#ifdef __TURBOC__
   #include <mem.h>
   #include <alloc.h>
#else
   #include <memory.h>
   #include <malloc.h>
#endif
#include "inter.h"

void  (*_CloseInterface)(void);
void  (*_DefineColor)(unsigned color,Rgb rgb);
void  (*_DefinePair)(unsigned pair,unsigned foreground,unsigned background);
void  (*_UpdateCell)(unsigned x,unsigned y,unsigned offs,ScreenCell *cell);
int   (*_KeyPressed)(void);
unsigned (*_ReadKey)(int timeout);
void  (*_SetCursor)(unsigned x,unsigned y,int type);
void  (*_Sync)(void);
void  (*_Suspend)(void);
void  (*_Resume)(void);
void  (*_Redraw)(void);
void  (*_Bell)(void);
unsigned (*_Timer)(void);

unsigned I_ScreenWidth;      /* dimensions of the screen */
unsigned I_ScreenHeight;
unsigned I_ScreenSize;
unsigned I_ScreenType;       /* type of the screen */
unsigned I_ScreenFlags;      /* flags of the screen */

ScreenCell  *_I_Screen;       /* local screen contents */

Rgb         *_I_ColorMap;     /* current color values */
RgbState    *_I_ColorInfo;
unsigned    I_ColorsNum;     /* number of colors in color map */
Pair        *_I_PairMap;
PairState   *_I_PairInfo;
unsigned    I_PairsNum;

unsigned    I_CursorX;
unsigned    I_CursorY;
int         I_CurType;
int         I_MouseX;
int         I_MouseY;
unsigned    _I_MouseOffs;
unsigned    I_LastButton;
long        I_LastTime;      /* time of last event in milliseconds */
unsigned    I_LastKey;
unsigned    I_Buttons;

int         _I_OldMouseX;
int         _I_OldMouseY;
unsigned    _I_OldMouseOffs;

int         _I_HaveReadKey;

unsigned    I_EventMask;
/*____________________________________________________________________________
*/

static
long  CalcDist(Rgb c1,Rgb c2)
{
   long  Dist1;
   long  Dist2;
   long  Dist3;

   Dist1=c1.r-c2.r;
   Dist1*=Dist1;
   Dist2=c1.g-c2.g;
   Dist2*=Dist2;
   Dist3=c1.b-c2.b;
   Dist3*=Dist3;

   return(Dist1+Dist2+Dist3);
}

static
unsigned AllocColor(Rgb color)
{
   unsigned Num;
   unsigned Best;
   long     Dist;
   long     BestDist;

   for(Num=0; Num<I_ColorsNum; Num++)
   {
      if(!memcmp(&color,&_I_ColorMap[Num],sizeof(Rgb)))
      {
         _I_ColorInfo[Num].refs++;
         return(Num);
      }
   }
   if(I_ScreenFlags&CAN_CHANGE_COLOR)
   {
      for(Num=0; Num<I_ColorsNum; Num++)
      {
         if(_I_ColorInfo[Num].refs==0 && !(_I_ColorInfo[Num].flags&READONLY))
         {
            _I_ColorInfo[Num].flags=MODIFIED;
            _I_ColorInfo[Num].refs=1;
            _I_ColorMap[Num]=color;
            return(Num);
         }
      }
   }

   Best=0;
   BestDist=CalcDist(color,_I_ColorMap[0]);
   for(Num=1; Num<I_ColorsNum; Num++)
   {
      Dist=CalcDist(color,_I_ColorMap[Num]);
      if(Dist<BestDist)
      {
         BestDist=Dist;
         Best=Num;
      }
   }
   _I_ColorInfo[Best].refs++;
   return(Best);
}

unsigned AllocPair(unsigned foreground,unsigned background)
{
   unsigned pair;
   unsigned Best;
   unsigned Dist;
   unsigned BestDist;

   for(pair=0; pair<I_PairsNum; pair++)
   {
      if(_I_PairMap[pair].foreground==foreground &&
         _I_PairMap[pair].background==background)
      {
         _I_PairInfo[pair].refs++;
         return(pair);
      }
   }
   for(pair=0; pair<I_PairsNum; pair++)
   {
      if(_I_PairInfo[pair].refs==0 && !(_I_PairInfo[pair].flags&READONLY))
      {
         _I_PairInfo[pair].refs++;
         _I_PairInfo[pair].flags=MODIFIED;
         _I_PairMap[pair].foreground=foreground;
         _I_PairMap[pair].background=background;
         return(pair);
      }
   }

   Best=0;
   BestDist=CalcDist(_I_ColorMap[foreground],_I_ColorMap[_I_PairMap[0].foreground])+
            CalcDist(_I_ColorMap[background],_I_ColorMap[_I_PairMap[0].background]);
   for(pair=1; pair<I_PairsNum; pair++)
   {
      Dist=CalcDist(_I_ColorMap[foreground],_I_ColorMap[_I_PairMap[pair].foreground])+
           CalcDist(_I_ColorMap[background],_I_ColorMap[_I_PairMap[pair].background]);
      if(Dist<BestDist)
      {
         BestDist=Dist;
         Best=pair;
      }
   }
   _I_PairInfo[Best].refs++;
   return(Best);
}

void  SetScreenCell(unsigned x,unsigned y,Cell *NewCell)
{
   register ScreenCell  *ScCell;

   ScCell=_I_Screen+x+y*I_ScreenWidth;

   if(ScCell->ch==NewCell->ch
   && (((ScCell->flags&~(MODIFIED|COLORALLOCATED))==NewCell->flags)
       && (!(ScCell->flags&COLOR)
           || (!memcmp(&ScCell->foregroundrgb,&(NewCell->foreground),sizeof(Rgb))
               && !memcmp(&ScCell->backgroundrgb,&(NewCell->background),sizeof(Rgb))))))
      return;

   if(ScCell->flags&COLORALLOCATED)
   {
      _I_ColorInfo[ScCell->foreground].refs--;
      _I_ColorInfo[ScCell->background].refs--;
      if(I_ScreenFlags&HAS_PAIRS)
         _I_PairInfo[ScCell->pair].refs--;
      ScCell->flags&=~COLORALLOCATED;
   }

   ScCell->ch=NewCell->ch;
   ScCell->flags=(NewCell->flags|MODIFIED)&~COLORALLOCATED;

   if(ScCell->flags&COLOR && I_ScreenFlags&HAS_COLORS)
   {
      ScCell->foregroundrgb=NewCell->foreground;
      ScCell->backgroundrgb=NewCell->background;
   }
}

void  GetScreenCell(unsigned x,unsigned y,Cell *c)
{
   ScreenCell  *sc=&_I_Screen[x+y*I_ScreenWidth];
   c->flags=sc->flags&~(MODIFIED|COLORALLOCATED);
   if(sc->flags&COLOR)
   {
      c->foreground=sc->foregroundrgb;
      c->background=sc->backgroundrgb;
   }
   c->ch=sc->ch;
}

void  Sync()
{
   register ScreenCell  *ScCell;
   unsigned             x,y;
   register unsigned    offs;

   /* allocate requested colors */
   if(I_ScreenFlags&HAS_COLORS)
   {
      for(ScCell=_I_Screen,offs=0; offs<I_ScreenSize; ScCell++,offs++)
      {
         if(!(ScCell->flags&COLORALLOCATED) && (ScCell->flags&COLOR))
         {
            ScCell->foreground=AllocColor(ScCell->foregroundrgb);
            ScCell->background=AllocColor(ScCell->backgroundrgb);
            if(I_ScreenFlags&HAS_PAIRS)
               ScCell->pair=AllocPair(ScCell->foreground,ScCell->background);
            ScCell->flags|=COLORALLOCATED;
         }
      }
      if(I_ScreenFlags&CAN_CHANGE_COLOR)
      {
         for(offs=0; offs<I_ColorsNum; offs++)
            if(_I_ColorInfo[offs].flags&MODIFIED)
            {
               _DefineColor(offs,_I_ColorMap[offs]);
               _I_ColorInfo[offs].flags&=~MODIFIED;
            }
      }
      if(I_ScreenFlags&HAS_PAIRS)
      {
         for(offs=0; offs<I_PairsNum; offs++)
         {
            if(_I_PairInfo[offs].flags&MODIFIED)
            {
               _DefinePair(offs,_I_PairMap[offs].foreground,_I_PairMap[offs].background);
               _I_PairInfo[offs].flags&=~MODIFIED;
            }
         }
      }
   }

   ScCell=_I_Screen;
   offs=0;
   x=y=0;

   while(offs<I_ScreenSize)
   {
      if(ScCell->flags&MODIFIED)
      {
         if(offs==_I_MouseOffs && I_ScreenFlags&SHOW_MOUSE)
            ScCell->flags^=REVERSE;
         _UpdateCell(x,y,offs,ScCell);
         ScCell->flags&=~MODIFIED;
         if(offs==_I_MouseOffs && I_ScreenFlags&SHOW_MOUSE)
            ScCell->flags^=REVERSE;
      }
      offs++;
      ScCell++;
      if(++x>=I_ScreenWidth)
      {
         x=0;
         y++;
      }
   }
   _SetCursor(I_CursorX,I_CursorY,I_CurType);
   _Sync();
}

void  CloseInterface()
{
   Sync();
   _CloseInterface();
   free(_I_Screen);
   free(_I_ColorMap);
   free(_I_ColorInfo);
   free(_I_PairMap);
   free(_I_PairInfo);
}

void     ShowMouse(int flag)
{
   ScreenCell  *ScCell;

   if(flag)
   {
      if((I_ScreenFlags&HAS_MOUSE) && !(I_ScreenFlags&SHOW_MOUSE))
      {
         ScCell=_I_Screen+_I_MouseOffs;
         ScCell->flags^=REVERSE;
         _UpdateCell(I_MouseX,I_MouseY,_I_MouseOffs,ScCell);
         ScCell->flags&=~MODIFIED;
         ScCell->flags^=REVERSE;
         I_ScreenFlags|=SHOW_MOUSE;
      }
   }
   else
   {
      if(I_ScreenFlags&SHOW_MOUSE)
      {
         ScCell=_I_Screen+_I_MouseOffs;
         _UpdateCell(I_MouseX,I_MouseY,_I_MouseOffs,ScCell);
         ScCell->flags&=~MODIFIED;
         I_ScreenFlags&=~SHOW_MOUSE;
      }
   }
}

void  BoundMouse(void)
{
   if(I_MouseX<0)
      I_MouseX=0;
   else if(I_MouseX>=I_ScreenWidth)
      I_MouseX=I_ScreenWidth-1;
   if(I_MouseY<0)
      I_MouseY=0;
   else if(I_MouseY>=I_ScreenHeight)
      I_MouseY=I_ScreenHeight-1;
}

static int  HandleLastKey(void)
{
   ScreenCell  *ScCell;

   if(I_LastKey==M_MOVE)
   {
      BoundMouse();
      _I_MouseOffs=I_MouseX+I_ScreenWidth*I_MouseY;
      if(_I_MouseOffs!=_I_OldMouseOffs)
      {
         if(I_ScreenFlags&SHOW_MOUSE)
         {
            ScCell=_I_Screen+_I_MouseOffs;
            ScCell->flags^=REVERSE;
            _UpdateCell(I_MouseX,I_MouseY,_I_MouseOffs,ScCell);
            ScCell->flags&=~MODIFIED;
            ScCell->flags^=REVERSE;
            ScCell=_I_Screen+_I_OldMouseOffs;
            _UpdateCell(_I_OldMouseX,_I_OldMouseY,_I_OldMouseOffs,ScCell);
            ScCell->flags&=~MODIFIED;
            _SetCursor(I_CursorX,I_CursorY,I_CurType);
            _Sync();
         }
         _I_OldMouseX=I_MouseX;
         _I_OldMouseY=I_MouseY;
         _I_OldMouseOffs=_I_MouseOffs;
      }
      else
         I_LastKey=M_MOVE1;    /* mouse was moved but had no effect */
   }
   else if(I_LastKey==M_BUTTON)
      I_Buttons|=I_LastButton;
   else if(I_LastKey==M_RBUTTON)
      I_Buttons&=~I_LastButton;

   return(_I_HaveReadKey=((I_EventMask&KEYBOARD_EVENTS && IsKeyboardEvent(I_LastKey))
                       || (I_EventMask&MOUSE_EVENTS && IsMouseEvent(I_LastKey))
                       || (I_EventMask&OTHER_EVENTS && IsOtherEvent(I_LastKey))));
}

int   KeyPressed()
{
   if(_I_HaveReadKey)
      return(TRUE);

   while(_KeyPressed())
   {
      I_LastKey=_ReadKey(0);
      if(HandleLastKey())
         return(TRUE);
   }
   return(FALSE);
}

unsigned WaitKey(int timeout)
{
   unsigned long  start_timer=Timer();
   unsigned long  curr_timer;

   if(timeout<0)
      return(ReadKey());

   if(!KeyPressed())
      Sync();

   for(;;)
   {
      if(_I_HaveReadKey)
      {
         _I_HaveReadKey=FALSE;
         return(I_LastKey);
      }
      curr_timer=Timer();
      if(curr_timer-start_timer>=timeout)
         break;
      I_LastKey=_ReadKey(timeout-(curr_timer-start_timer));
      HandleLastKey();
   }
   return(I_LastKey=K_NONE);
}

unsigned ReadKey()
{
   if(!KeyPressed())
      Sync();

   for(;;)
   {
      if(_I_HaveReadKey)
      {
         _I_HaveReadKey=FALSE;
         return(I_LastKey);
      }
      I_LastKey=_ReadKey(-1);
      HandleLastKey();
   }
   /*NOTREACHED*/
}

void  MoveCursor(unsigned x,unsigned y)
{
   if(x<I_ScreenWidth && y<I_ScreenHeight)
   {
      I_CursorX=x;
      I_CursorY=y;
   }
}

void  CursorType(int type)
{
	I_CurType=type;
}

void  SuspendInterface(void)
{
	if(_Suspend)
		_Suspend();
}

void  ResumeInterface(void)
{
	if(_Resume)
		_Resume();
	RedrawScreen();
}

void  RedrawScreen(void)
{
	if(_Redraw)
      _Redraw();
   else
   {
      int   offs;
      for(offs=0; offs<I_ScreenSize; offs++)
         _I_Screen[offs].flags|=MODIFIED;
      for(offs=0; offs<I_ColorsNum; offs++)
         _I_ColorInfo[offs].flags|=MODIFIED;
      for(offs=0; offs<I_PairsNum; offs++)
         _I_PairInfo[offs].flags|=MODIFIED;
	}
}

void  ClearScreen(void)
{
   int   x,y;
   Cell  blank={0,' '};

   for(y=0; y<I_ScreenHeight; y++)
      for(x=0; x<I_ScreenWidth; x++)
         SetScreenCell(x,y,&blank);
}

void  Bell(void)
{
   Sync();
   if(_Bell)
      _Bell();
}

void  SetEventMask(unsigned mask)
{
   I_EventMask=mask;
}

unsigned K_F(int n)
{
   static unsigned   table[]=
      {K_F1,K_F2,K_F3,K_F4,K_F5,K_F6,
      K_F7,K_F8,K_F9,K_F10,K_F11,K_F12};
   return(table[n-1]);
}
unsigned K_Shift_F(int n)
{
   static unsigned   table[]=
      {K_SHIFT_F1,K_SHIFT_F2,K_SHIFT_F3,K_SHIFT_F4,K_SHIFT_F5,K_SHIFT_F6,
      K_SHIFT_F7,K_SHIFT_F8,K_SHIFT_F9,K_SHIFT_F10,K_SHIFT_F11,K_SHIFT_F12};
   return(table[n-1]);
}
unsigned K_Ctrl_F(int n)
{
   static unsigned   table[]=
      {K_CTRL_F1,K_CTRL_F2,K_CTRL_F3,K_CTRL_F4,K_CTRL_F5,K_CTRL_F6,
      K_CTRL_F7,K_CTRL_F8,K_CTRL_F9,K_CTRL_F10,K_CTRL_F11,K_CTRL_F12};
   return(table[n-1]);
}
unsigned K_Alt_F(int n)
{
   static unsigned   table[]=
      {K_ALT_F1,K_ALT_F2,K_ALT_F3,K_ALT_F4,K_ALT_F5,K_ALT_F6,
      K_ALT_F7,K_ALT_F8,K_ALT_F9,K_ALT_F10,K_ALT_F11,K_ALT_F12};
   return(table[n-1]);
}
