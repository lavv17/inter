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

#ifdef DEBUG
#include <stdarg.h>

void  debug(char *s,...)
{
   va_list  p;
   va_start(p,s);
   vprintf(s,p);
   va_end(p);
   putchar('\n');
   fflush(stdout);
   getch();
}
#endif

short peek(unsigned short seg,unsigned short offs)
{
   short res;
   dosmemget((seg<<4)+offs,2,&res);
   return(res);
}
void  poke(unsigned short seg,unsigned short offs,short value)
{
   dosmemput(&value,2,(seg<<4)+offs);
}
char  peekb(unsigned short seg,unsigned short offs)
{
   char res;
   dosmemget((seg<<4)+offs,1,&res);
   return(res);
}
void  pokeb(unsigned short seg,unsigned short offs,char value)
{
   dosmemput(&value,1,(seg<<4)+offs);
}
#endif

static
unsigned VideoMemSeg,VideoMemOffs;
static
int   old_mouse_x,old_mouse_y,old_buttons;

#define  QLEN  256
struct   DosEvent
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
};
static struct
DosEvent DosEventQueue[QLEN];
static
int   QueueHead,QueueTail;

static
int   AppendQueue(struct DosEvent *ev)
{
   int   NewTail;
   
   disable();
   ev->time=Timer();
   NewTail=QueueTail+1;
   if(NewTail>=QLEN)
      NewTail=0;
   if(NewTail!=QueueHead)
   {
      DosEventQueue[QueueTail]=*ev;
      QueueTail=NewTail;
      enable();
      return(1);
   }
   enable();
   return(0);
}
int   EventQueueEmpty(void)
{
   return(QueueHead==QueueTail);
}

void  ReadInEvents(void)
{
   struct
   DosEvent newevent;
   union REGS  r;
   int   new_mouse_x,new_mouse_y,new_buttons;

   disable();
   while(peek(0,0x41A)!=peek(0,0x41C))
   {
      newevent.type=/*bioskey(1);*/peek(0x40,peek(0,0x41A));
      newevent.time=Timer();
      enable();
      if((newevent.type&0x00FF)==0x00E0 && (newevent.type&0xFF00)>=0x3B00)
         newevent.type&=0xFF00;
      else if((newevent.type&0x00FF)!=0 || newevent.type==0x0300)
         newevent.type&=255;
      if(AppendQueue(&newevent))
         bioskey(0);
      disable();
   }
   enable();
   
   r.w.ax=3;
   int86(0x33,&r,&r);  /* qry pos & butt */
   
   new_mouse_x=(r.w.cx+4)/8;
   new_mouse_y=(r.w.dx+4)/8;
   new_buttons=((r.w.bx&1)?LEFT_BUTTON:0)
               |((r.w.bx&2)?RIGHT_BUTTON:0)
               |((r.w.bx&4)?CENTER_BUTTON:0);

   if(new_mouse_x!=old_mouse_x || new_mouse_y!=old_mouse_y) /* mouse movement */
   {
      newevent.type=M_MOVE;
      newevent.time=Timer();
      newevent.info.coord.x=new_mouse_x;
      newevent.info.coord.y=new_mouse_y;
      if(AppendQueue(&newevent))
      {
         old_mouse_x=new_mouse_x;
         old_mouse_y=new_mouse_y;
      }
   }
   if((old_buttons&LEFT_BUTTON) && !(new_buttons&LEFT_BUTTON))
   {
      newevent.type=M_RBUTTON;
      newevent.time=Timer();
      newevent.info.code=LEFT_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons&=~LEFT_BUTTON;
   }
   if(!(old_buttons&LEFT_BUTTON) && (new_buttons&LEFT_BUTTON))
   {
      newevent.type=M_BUTTON;
      newevent.time=Timer();
      newevent.info.code=LEFT_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons|=LEFT_BUTTON;
   }
   if((old_buttons&RIGHT_BUTTON) && !(new_buttons&RIGHT_BUTTON))
   {
      newevent.type=M_RBUTTON;
      newevent.time=Timer();
      newevent.info.code=RIGHT_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons&=~RIGHT_BUTTON;
   }
   if(!(old_buttons&RIGHT_BUTTON) && (new_buttons&RIGHT_BUTTON))
   {
      newevent.type=M_BUTTON;
      newevent.time=Timer();
      newevent.info.code=RIGHT_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons|=RIGHT_BUTTON;
   }
   if((old_buttons&CENTER_BUTTON) && !(new_buttons&CENTER_BUTTON))
   {
      newevent.type=M_RBUTTON;
      newevent.time=Timer();
      newevent.info.code=CENTER_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons&=~CENTER_BUTTON;
   }
   if(!(old_buttons&CENTER_BUTTON) && (new_buttons&CENTER_BUTTON))
   {
      newevent.type=M_RBUTTON;
      newevent.time=Timer();
      newevent.info.code=CENTER_BUTTON;
      if(AppendQueue(&newevent))
         old_buttons|=CENTER_BUTTON;
   }
}
unsigned DosReadKey(int timeout)
{
   unsigned key;
   unsigned long start_timer=Timer();

   for(;;)
   {
      ReadInEvents();
      if(!EventQueueEmpty())
         break;
      if(timeout>=0 && Timer()-start_timer>=timeout)
         return(K_NONE);
   }

   switch(DosEventQueue[QueueHead].type)
   {
      case(M_MOVE):
         I_MouseX=DosEventQueue[QueueHead].info.coord.x;
         I_MouseY=DosEventQueue[QueueHead].info.coord.y;
         break;
      case(M_BUTTON):
      case(M_RBUTTON):
         I_LastButton=DosEventQueue[QueueHead].info.code;
         break;
   }
   I_LastTime=DosEventQueue[QueueHead].time;
   key=DosEventQueue[QueueHead].type;
   if(++QueueHead>=QLEN)
      QueueHead=0;

   return(key);
}

int   DosKeyPressed(void)
{
   ReadInEvents();
   return(!EventQueueEmpty());
}

void  DosSync(void) {}

void  DosBell(void)
{
   union REGS  r;
   r.w.ax=0x0E07;
   int86(0x10,&r,&r);
}

void  DosUpdateCell(unsigned x,unsigned y,unsigned offs,ScreenCell *cell)
{
   unsigned attr;

   (void)x;
   (void)y;

   if(cell->flags&COLOR)
      attr=cell->foreground|(cell->background<<4);
   else
   {
      attr=7;
      if(cell->flags&BOLD)
         attr|=0x08;
      if(cell->flags&REVERSE)
         attr=(attr>>4)|(attr<<4);
   }
   poke(VideoMemSeg,VideoMemOffs+(offs<<1),(unsigned char)(cell->ch)+(attr<<8));
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
         r.w.cx=0x0607;
      else
      {
         r.h.ch=(font*11+8)>>4;
         r.h.cl=(font*13+8)>>4;
      }
   }
   else if(type==C_LARGE)
   {
      r.w.cx=0x001F;
   }
   else
   {
      r.w.cx=0x2020;
   }
   r.h.ah=1;
   int86(0x10,&r,&r);
}

static
void  SetBlink(int on)
{
   union REGS  r;
   r.w.ax=0x1003;
   r.h.bl=on;
   int86(0x10,&r,&r);
}

static
void  ResetScreen(void)
{
   union REGS  r;
   int   i;

   r.w.ax=0x0600;
   r.w.cx=0;
   r.h.dh=I_ScreenHeight-1;
   r.h.dl=I_ScreenWidth-1;
   r.h.bh=7;
   int86(0x10,&r,&r);

   DosSetCursor(0,0,C_NORMAL);

   if(I_ScreenType==S_EGA || I_ScreenType==S_VGA)
   {
      for(i=0; i<16; i++)
      {
         r.w.ax=0x1000;
         r.h.bl=i;
         r.h.bh=i%8;
         if(i&8)
            r.h.bh+=070;
         int86(0x10,&r,&r);

         if(I_ScreenType==S_VGA)
         {
            r.w.ax=0x1010;
            r.w.bx=i;
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
                  r.w.bx+=48;
               }
            }
            else
            {
               r.h.dh=0xAA;
               r.h.ch=0x55;
               r.h.cl=0;
            }
            int86(0x10,&r,&r);
         }
      }
   }
}

void  DosSuspend(void)
{
   ResetScreen();
   SetBlink(1);
/*   DeinstallHandlers();*/
}

void  DosResume(void)
{
/*   InstallHandlers();*/
   SetBlink(0);
}

void  DosCloseInterface(void)
{
/*   DeinstallHandlers();*/
   ResetScreen();
   SetBlink(1);
}

void  DosEgaDefineColor(unsigned color, Rgb rgb)
{
   union REGS  r;
   union REGS  r1;

   r.w.ax=0x1000; /* set one palette register */
   r.h.bl=color;
   r.h.bh=(((rgb.r>>1)&040) | ((rgb.g>>2)&020) | ((rgb.b>>3)&010)
    | ((rgb.r>>5)&004) | ((rgb.g>>6)&002) | ((rgb.b>>7)&001));
   int86(0x10,&r,&r1);
#if 0
   r.h.bh|=070;        /* provide bold color */
   r.h.bl+=8;
   int86(0x10,&r,&r);
#endif
}
void  DosVgaDefineColor(unsigned color, Rgb rgb)
{
   union REGS  r;
   union REGS  r1;

   r.w.ax=0x1010;
   r.w.bx=(color>=8)?color+48:color;
   r.h.dh=rgb.r>>2;
   r.h.ch=rgb.g>>2;
   r.h.cl=rgb.b>>2;
   int86(0x10,&r,&r1);
#if 0
   r.w.bx+=64-8;        /* provide bold color */
   r.h.dh|=0x15;
   r.h.ch|=0x15;
   r.h.cl|=0x15;
   int86(0x10,&r,&r1);
#endif
}

void  DosInit(void)
{
   union REGS  r;
   unsigned equip;

   I_ScreenFlags=CONSOLE;
   QueueHead=QueueTail=0;

   r.h.ah=15;
   int86(0x10,&r,&r);

   if(r.h.al==7)
      VideoMemSeg=0xB000L;
   else
      VideoMemSeg=0xB800L;

   equip=peek(0,0x410);
   if((equip&0x30)==0x30)
   {
      I_ScreenType=S_MDA;
   }
   else
   {
      r.w.ax=0x1A00;
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
            I_ColorsNum=16;
            break;
         case(5):
            I_ScreenType=S_EGA;
            I_ColorsNum=16;
            break;
         case(6):
            I_ScreenFlags|=HAS_COLORS;
            I_ColorsNum=8;
            break;
         case(7):
            I_ScreenType=S_VGA;
            I_ColorsNum=16;
            break;
         case(8):
            I_ScreenType=S_VGA;
            I_ScreenFlags|=HAS_COLORS/*|CAN_CHANGE_COLOR*/;
            I_ColorsNum=16;
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

      CheckPtr(_I_ColorMap=(Rgb*)calloc(I_ColorsNum,sizeof(*_I_ColorMap)));
      CheckPtr(_I_ColorInfo=(RgbState*)calloc(I_ColorsNum,sizeof(*_I_ColorInfo)));
      for(i=0; i<I_ColorsNum; i++)
      {
         _I_ColorMap[i].r=((i>>2)&1)*0xAA;
         _I_ColorMap[i].g=((i>>1)&1)*0xAA;
         _I_ColorMap[i].b=(i&1)*0xAA;
         if(i>=16)
         {
            _I_ColorMap[i].r+=0x55;
            _I_ColorMap[i].g+=0x55;
            _I_ColorMap[i].b+=0x55;
         }
         if(i==0 || i==7 || i==15)
            _I_ColorInfo[i].flags|=READONLY;
      }
   }

   r.w.ax=0;
   int86(0x33,&r,&r);   /* check mouse presence */
   if(r.w.ax==0xFFFF)
   {
      I_ScreenFlags|=HAS_MOUSE;
      r.w.ax=3;  /* query mouse pos and buttons */
      int86(0x33,&r,&r);
      old_mouse_x=I_MouseX=(r.w.cx+4)/8;
      old_mouse_y=I_MouseY=(r.w.dx+4)/8;
      old_buttons=I_Buttons=((r.w.bx&1)?LEFT_BUTTON:0)
                           |((r.w.bx&2)?RIGHT_BUTTON:0)
                           |((r.w.bx&4)?CENTER_BUTTON:0);
   }

/*   InstallHandlers();*/

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
   SetBlink(0);
}
