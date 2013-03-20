/*____________________________________________________________________________
**
** File:          idos.c
**
** Description:   DOS interface
**____________________________________________________________________________
*/

#if !defined(__BORLANDC__)
/*   #error   This module must be compiled with Borland C++ */
#endif

#include <stdio.h>
#include <malloc.h>
#include <bios.h>
#include <dos.h>
#include "inter.h"
#include "check.h"

#ifdef __GNUC__
#include <go32.h>

short peek(unsigned short seg,unsigned short offs)
{
   short res;
   dosmemget((seg<<4)+offs,2,&res);
   return(res);
}
void  poke(unsigned short seg,unsigned short offs,short value)
{
   dosmemput(&value,(seg<<4)+offs,2);
}
char  peekb(unsigned short seg,unsigned short offs)
{
   char res;
   dosmemget((seg<<4)+offs,1,&res);
   return(res);
}
void  pokeb(unsigned short seg,unsigned short offs,char value)
{
   dosmemput(&value,(seg<<4)+offs,1);
}
#endif

static
unsigned VideoMemSeg,VideoMemOffs;

#define  QLEN  256
static   volatile struct   DosEvent
{
   long     time;
   unsigned type;
   union
   {
      unsigned code;
      struct
      {
         int   x,y;
      } coord;
   } info;
}  DosEventQueue[QLEN];
static   volatile int   QueueHead;
static   volatile int   QueueTail;

static   void interrupt (*OldKeybdHandler)(void);

void  AppendQueue(struct DosEvent *ev)
{
   int   NewHead;
   disable();
   ev->time=Timer();
   NewHead=QueueHead+1;
   if(NewHead>=QLEN)
      NewHead=0;
   if(NewHead!=QueueTail)
   {
      DosEventQueue[QueueHead]=*ev;
      QueueHead=NewHead;
   }
   enable();
}
int   EventQueueEmpty(void)
{
   return(QueueHead==QueueTail);
}

void  interrupt   DosKeybdHandler(void)
{
   static struct DosEvent  newevent;

   poke(0,0x41a,peek(0,0x41c));  /* clear keyboard buffer */
   OldKeybdHandler();
   if(peek(0,0x41a)!=peek(0,0x41c)) /* there is a key code */
   {
      newevent.type=bioskey(1);
      if((newevent.type&255)!=0 || newevent.type==0x0300)
         newevent.type&=255;
      AppendQueue(&newevent);
   }
   poke(0,0x41a,peek(0,0x41c));  /* clear keyboard buffer */
}

void  far _loadds _saveregs DosMouseEventHandler(void)
{
   static struct DosEvent   newevent;
   int   x=_CX,y=_DX;
   unsigned mask=_AX;

   disable();

   while(mask)
   {
      if(mask&1)  /* mouse movement */
      {
         newevent.type=M_MOVE;
         newevent.info.coord.x=x;
         newevent.info.coord.y=y;
         mask&=~1;
      }
      else
      {
         if(mask&(2|8|32))
            newevent.type=M_BUTTON;
         else
            newevent.type=M_RBUTTON;
         if(mask&(2|4))
         {
            newevent.info.code=LEFT_BUTTON;
            mask&=~(2|4);
         }
         else if(mask&(8|16))
         {
            newevent.info.code=RIGHT_BUTTON;
            mask&=~(8|16);
         }
         else if(mask&(32|64))
         {
            newevent.info.code=MIDDLE_BUTTON;
            mask&=~(32|64);
         }
         else
            break;
      }
      AppendQueue(&newevent);
   }
}

void  InstallHandlers(void)
{
   union    REGS  r;
   struct   SREGS sr;

   if(I_ScreenFlags&HAS_MOUSE)
   {
      r.x.ax=0;
      int86(0x33,&r,&r);         /* reset mouse */
      r.x.ax=7;
      r.x.cx=0;
      r.x.dx=(I_ScreenWidth<<3)-1;
      int86(0x33,&r,&r);         /* set horizontal range for mouse */
      r.x.ax=8;
      r.x.cx=0;
      r.x.dx=(I_ScreenHeight<<3)-1;
      int86(0x33,&r,&r);         /* set vertical range for mouse */

      r.x.ax=3;
      int86(0x33,&r,&r);         /* get position and buttons */
      I_MouseX=(r.x.cx+4)>>3;
      I_MouseY=(r.x.dx+4)>>3;
      I_Buttons=((r.x.bx&1)?LEFT_BUTTON:0)|((r.x.bx&2)?RIGHT_BUTTON:0)|
              ((r.x.bx&4)?MIDDLE_BUTTON:0);

      r.x.ax=0x0c;
      r.x.cx=0x7f;
      segread(&sr);
      sr.es=FP_SEG(DosMouseEventHandler);
      r.x.dx=FP_OFF(DosMouseEventHandler);
      int86x(0x33,&r,&r,&sr);             /* install mouse event handler */
   }

   OldKeybdHandler=getvect(9);
   setvect(9,DosKeybdHandler);
}
void  DeinstallHandlers(void)
{
   union    REGS  r;
   struct   SREGS sr;

   if(I_ScreenFlags&HAS_MOUSE)
   {
      r.x.ax=0x0c;
      r.x.cx=0;
      segread(&sr);
      sr.es=FP_SEG(DosMouseEventHandler);
      r.x.bx=FP_OFF(DosMouseEventHandler);
      int86x(0x33,&r,&r,&sr);             /* deinstall mouse event handler */
   }

   setvect(9,OldKeybdHandler);
}

unsigned DosReadKey(int timeout)
{
   unsigned key;
   unsigned long start_timer=Timer();

   enable();
   while(EventQueueEmpty())   /* wait for an event */
   {
      if(timeout>=0 && Timer()-start_timer>=timeout)
         return(K_NONE);
   }

   disable();
   switch(DosEventQueue[QueueTail].type)
   {
      case(M_MOVE):
         I_MouseX=(DosEventQueue[QueueTail].info.coord.x+4)>>3;
         I_MouseY=(DosEventQueue[QueueTail].info.coord.y+4)>>3;
         break;
      case(M_BUTTON):
      case(M_RBUTTON):
         I_LastButton=DosEventQueue[QueueTail].info.code;
         break;
   }
   I_LastTime=DosEventQueue[QueueTail].time;
   key=DosEventQueue[QueueTail].type;
   if(++QueueTail>=QLEN)
      QueueTail=0;

   enable();
   return(key);
}

int   DosKeyPressed(void)
{
   return(!EventQueueEmpty());
}

void  DosSync(void) {}

void  DosBell(void)
{
   union REGS  r;
   r.x.ax=0x0E07;
   int86(0x10,&r,NULL);
}

void  DosUpdateCell(unsigned x,unsigned y,unsigned offs,ScreenCell *cell)
{
   unsigned char  attr=0x07;

   (void)x;
   (void)y;

   if(cell->flags&COLOR)
      attr=cell->foreground|(cell->background<<4);
   if(cell->flags&REVERSE)
      attr=(attr>>4)|(attr<<4);
   if(cell->flags&BOLD)
      attr|=0x08;
   if(cell->flags&BLINK)
      attr|=0x80;

   poke(VideoMemSeg,offs<<1,(unsigned char)(cell->ch)+(attr<<16));
}

void  DosSetCursor(unsigned x,unsigned y,int type)
{
   union REGS  r;

   r.h.ah=15;
   int86(0x10,&r,&r);
   r.h.ah=2;
   r.h.dl=x;
   r.h.dh=y;
   int86(0x10,&r,&r);

   if(type==C_NORMAL)
   {
      int   font=peek(0,0x485);
      if(font==0)
         r.x.cx=0x0607;
      else
      {
         r.h.ch=(font*11+8)>>4;
         r.h.cl=(font*13+8)>>4;
      }
   }
   else if(type==C_LARGE)
   {
      r.x.cx=0x001F;
   }
   else
   {
      r.x.cx=0x2020;
   }
   r.h.ah=1;
   int86(0x10,&r,&r);
}

static   void  ResetScreen(void)
{
   union REGS  r;
   int   i;

   if(I_ScreenType==S_EGA || I_ScreenType==S_VGA)
   {
      for(i=0; i<16; i++)
      {
         r.x.ax=0x1000;
         r.h.bl=i;
         r.h.bh=i%8;
         if(i&8)
            r.h.bh+=070;
         int86(0x10,&r,NULL);

         if(I_ScreenType==S_VGA)
         {
            r.x.ax=0x1010;
            r.x.bx=i;
            if(i!=6)
            {
               r.h.dh=(i&4)?0xAA:0;
               r.h.ch=(i&2)?0xAA:0;
               r.h.cl=(i&1)?0xAA:0;
               if(i&8)
               {
                  r.h.dh+=0x55;
                  r.h.ch+=0x55;
                  r.h.cl+=0x55;
                  r.x.bx+=48;
               }
            }
            else
            {
               r.h.dh=0xAA;
               r.h.ch=0x55;
               r.h.cl=0;
            }
            int86(0x10,&r,NULL);
         }
      }
   }

   r.x.ax=0x0600;
   r.x.cx=0;
   r.h.dh=I_ScreenHeight-1;
   r.h.dl=I_ScreenWidth-1;
   r.h.bh=7;
   int86(0x10,&r,NULL);

   DosSetCursor(0,0,C_NORMAL);
}

void  DosSuspend(void)
{
   ResetScreen();
   DeinstallHandlers();
}

void  DosResume(void)
{
   InstallHandlers();
}

void  DosCloseInterface(void)
{
   DeinstallHandlers();
   ResetScreen();
}

void  DosEgaDefineColor(unsigned color, Rgb rgb)
{
   union REGS  r;

   r.x.ax=0x1000; /* set one palette register */
   r.h.bl=color;
   r.h.bh=(((rgb.r>>1)&040) | ((rgb.g>>2)&020) | ((rgb.b>>3)&010)
    | ((rgb.r>>5)&004) | ((rgb.g>>6)&002) | ((rgb.b>>7)&001));
   int86(0x10,&r,NULL);

   r.h.bh|=070;        /* provide bold color */
   r.h.bl+=8;
   int86(0x10,&r,NULL);
}
void  DosVgaDefineColor(unsigned color, Rgb rgb)
{
   union REGS  r;

   r.x.ax=0x1010;
   r.x.bx=color;
   r.h.dh=rgb.r>>2;
   r.h.ch=rgb.g>>2;
   r.h.cl=rgb.b>>2;
   int86(0x10,&r,NULL);

   r.x.bx+=64-8;        /* provide bold color */
   r.h.dh|=0x15;
   r.h.ch|=0x15;
   r.h.cl|=0x15;
   int86(0x10,&r,NULL);
}

void  DosInit(void)
{
   union REGS  r;

   I_ScreenFlags=CONSOLE;
   QueueHead=QueueTail=0;

   r.h.ah=15;
   int86(0x10,&r,&r);

   if(r.h.al==7)
      VideoMemSeg=0xB000L;
   else
      VideoMemSeg=0xB800L;

   int86(0x11,&r,&r);
   if((r.h.al&0x30)==0x30)
   {
      I_ScreenType=S_MDA;
   }
   else
   {
      r.x.ax=0x1A00;
      int86(0x10,&r,&r);   /* query display code */
      if(r.h.al==0x1A)     /* check if the function is supported */
      {
         switch(r.h.bl)
         {
         case(1):
            I_ScreenType=S_MDA;
            break;
         case(2):
            I_ScreenType=S_CGA;
            I_ScreenFlags|=HAS_COLORS;
            I_ColorsNum=8;
            break;
         case(4):
            I_ScreenType=S_EGA;
            I_ScreenFlags|=HAS_COLORS/*|CAN_CHANGE_COLOR*/;
            I_ColorsNum=8;
            break;
         case(5):
            I_ScreenType=S_EGA;
            break;
         case(6):
            I_ScreenFlags|=HAS_COLORS;
            I_ColorsNum=8;
            break;
         case(7):
            I_ScreenType=S_VGA;
            break;
         case(8):
            I_ScreenType=S_VGA;
            I_ScreenFlags|=HAS_COLORS/*|CAN_CHANGE_COLOR*/;
            I_ColorsNum=8;
            break;
         case(11):
            break;
         case(12):
            I_ScreenFlags|=HAS_COLORS;
            I_ColorsNum=8;
            break;
         }
      }
      else
      {
         VideoMemSeg=0xB800L;
         I_ScreenFlags|=HAS_COLORS;
         I_ColorsNum=8;
      }
   }
   if(I_ScreenType==S_MDA)
   {
      I_ScreenWidth=80;
      I_ScreenHeight=25;
   }
   else
   {
      VideoMemOffs=peek(0,0x44E);   /* add page offset */

      I_ScreenWidth=peek(0,0x44A);
      I_ScreenHeight=peekb(0,0x484)+1;
      if(I_ScreenHeight==1)
         I_ScreenHeight=peek(0,0x44C)/I_ScreenWidth/2;
   }
   I_ScreenSize=I_ScreenWidth*I_ScreenHeight;
   CheckPtr(_I_Screen=(ScreenCell*)calloc(I_ScreenSize,sizeof(*_I_Screen)));
   if(I_ScreenFlags&HAS_COLORS)
   {
      short i;

      CheckPtr(_I_ColorMap=(Rgb*)calloc(I_ColorsNum,sizeof(Rgb)));
      CheckPtr(_I_ColorInfo=(RgbState*)calloc(I_ColorsNum,sizeof(*_I_ColorInfo)));
      for(i=0; i<I_ColorsNum; i++)
      {
         _I_ColorMap[i].r=((i>>2)&1)*0xAA;
         _I_ColorMap[i].g=((i>>1)&1)*0xAA;
         _I_ColorMap[i].b=(i&1)*0xAA;
         if(i==0 || i==I_ColorsNum-1)
            _I_ColorInfo[i].flags|=READONLY;
      }
   }

   r.x.ax=0;
   int86(0x33,&r,&r);   /* check mouse presence */
   if(r.x.ax==0xFFFF)
   {
      I_ScreenFlags|=HAS_MOUSE;
      I_MouseX=I_ScreenWidth>>1;
      I_MouseY=I_ScreenHeight>>1;
   }

   InstallHandlers();

   _UpdateCell=DosUpdateCell;
   _CloseInterface=DosCloseInterface;
   _KeyPressed=DosKeyPressed;
   _ReadKey=DosReadKey;
   _SetCursor=DosSetCursor;
   _Sync=DosSync;
   _Redraw=NULL;
   _Suspend=DosSuspend;
   _Resume=DosResume;
   _Bell=DosBell;
   if(I_ScreenType==S_EGA)
   {
      _DefineColor=DosEgaDefineColor;
      I_ScreenFlags|=CAN_CHANGE_COLOR;
   }
   else if(I_ScreenType==S_VGA)
   {
      _DefineColor=DosVgaDefineColor;
      I_ScreenFlags|=CAN_CHANGE_COLOR;
   }

   ResetScreen();
}
