/*____________________________________________________________________________
**
** File:          iconsole.c
**
** Description:   interface for SCO console
**____________________________________________________________________________
*/

#include <stdio.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/vtkd.h>
#include <sys/console.h>
#include <sys/vid.h>
#include <sys/keyboard.h>
#include <sys/fcntl.h>
#include <termio.h>
#include <memory.h>
#include <prototypes.h>
#include <sys/event.h>
#include <mouse.h>
#include <poll.h>
#include "inter.h"
#include "check.h"

#define  SIG_REL  SIGUSR1
#define  SIG_ACQ  SIGUSR2

unsigned char  *VideoMem;
unsigned char  *VideoSave;
unsigned char  *PVideoMem;
void  rel_screen(), acq_screen();
strmap_t strmap;
struct   termio   oldterm;
struct   termio   newterm;
int   qfd;

static   UserVideoMode;
static   ProgramVideoMode;

static   MouseRX=8;
static   MouseRY=8;
static   MouseDX=0;
static   MouseDY=0;

static   font=16;

static   unsigned fkeys[]=
{
   K_F1,       K_F2,       K_F3,       K_F4,
   K_F5,       K_F6,       K_F7,       K_F8,
   K_F9,       K_F10,      K_F11,      K_F12,
   K_SHIFT_F1, K_SHIFT_F2, K_SHIFT_F3, K_SHIFT_F4,
   K_SHIFT_F5, K_SHIFT_F6, K_SHIFT_F7, K_SHIFT_F8,
   K_SHIFT_F9, K_SHIFT_F10,K_SHIFT_F11,K_SHIFT_F12,
   K_CTRL_F1,  K_CTRL_F2,  K_CTRL_F3,  K_CTRL_F4,
   K_CTRL_F5,  K_CTRL_F6,  K_CTRL_F7,  K_CTRL_F8,
   K_CTRL_F9,  K_CTRL_F10, K_CTRL_F11, K_CTRL_F12,
   K_ALT_F1,   K_ALT_F2,   K_ALT_F3,   K_ALT_F4,
   K_ALT_F5,   K_ALT_F6,   K_ALT_F7,   K_ALT_F8,
   K_ALT_F9,   K_ALT_F10,  K_ALT_F11,  K_ALT_F12,
   K_HOME,     K_UP,       K_PGUP,     K_G_MINUS,
   K_LEFT,     K_CENTER,   K_RIGHT,    K_G_PLUS,
   K_END,      K_DOWN,     K_PGDN,     K_INS
};

void  ConsoleSync(void)
{
   /* nothing to do */
}

void  ConsoleBell(void)
{
   putchar('\007');
   fflush(stdout);
}

void  ConsoleSetCursor(unsigned x,unsigned y,int type)
{
   int   high,low;
   if(type==C_INVISIBLE)
      high=low=32;
   else if(type==C_LARGE)
      high=0,low=31;
   else
   {
      high=(font*11+8)>>4;
      low=(font*13+8)>>4;
   }
   printf("\033[%d;%dH\033[=%d;%dC",y+1,x+1,high,low);
   fflush(stdout);
}

static   void  ConsoleClearScreen(void)
{
   printf("\033[H\033[J");
   ConsoleSetCursor(0,0,C_NORMAL);
}

static   void  ConsoleSuspend(void)
{
   struct   vt_mode  smode;

   memcpy(VideoSave,VideoMem,I_ScreenSize*2);
   ConsoleClearScreen();
   ioctl(0,TCGETA,&newterm);
   ev_suspend();

   smode.mode = VT_AUTO;
   ioctl(0, VT_SETMODE,&smode);

   ioctl(0,TCSETAF,&oldterm);
}

static   void  ConsoleResume(void)
{
   struct   vt_mode  smode;
   struct   vid_info vinfo;
   struct   m6845_info  minfo;

   if(ProgramVideoMode!=(UserVideoMode=ioctl(0,CONS_GET,0)))
      ioctl(0,MODESWITCH|ProgramVideoMode,0);

   vinfo.size=sizeof(vinfo);
   ioctl(0,CONS_GETINFO,&vinfo);

   minfo.size=sizeof(minfo);
   minfo.screen_top=0;
   ioctl(0,CONS_6845INFO,&minfo);

   if(vinfo.m_num!=ioctl(0,CONSADP,ioctl(0,CONS_CURRENT,0)))
      VideoMem=VideoSave;
   else
      VideoMem=PVideoMem+minfo.screen_top*2;

   memcpy(VideoMem,VideoSave,I_ScreenSize*2);

   ioctl(0,TCSETAF,&newterm);

   smode.mode = VT_PROCESS;
   smode.waitv = 0;    /* not implemented, reserved */
   smode.relsig = SIG_REL;
   smode.acqsig = SIG_ACQ;
   smode.frsig  = SIGINT;  /* not implemented, reserved */
   ioctl(0, VT_SETMODE,&smode);

   ev_resume();

   ioctl(0,GIO_STRMAP,strmap);
}

unsigned ConsoleReadKey(int timeout)
{
   static   unsigned buttons=0;
   unsigned diff;
   int      i;
   EVENT    *nev;
   EVENT    ev;
   int      keyno;
   char     *s;
   struct pollfd  pfd[1];

   for(;;)
   {
      pfd[0].fd=qfd;
      pfd[0].events=POLLIN|POLLPRI;
      if(poll(pfd,1,timeout)<1)
         return(K_NONE);
      if(ev_block()==-1)
         return(K_NONE);
      if((nev=ev_read())==NULL)
         return(K_NONE);
      ev=*nev;
      ev_pop();
      I_LastTime=EV_TIME(ev);

      if(EV_TAG(ev)&T_STRING)
      {
         if(EV_BUFCNT(ev)==1)
            return(EV_BUF(ev)[0]);  /* because it is a simple key */
         keyno=0;
         EV_BUF(ev)[EV_BUFCNT(ev)]=0;
         for(s=strmap; s<strmap+sizeof(strmap); s+=strlen(s)+1,keyno++)
            if(!strcmp(s,EV_BUF(ev)) && keyno<sizeof(fkeys)/sizeof(fkeys[0]))
               return(fkeys[keyno]);
      }
      if(EV_TAG(ev)&T_BUTTON)
      {
         diff=buttons^EV_BUTTONS(ev);
         if(diff&BUTTON1)
            I_LastButton=RIGHT_BUTTON;
         else if(diff&BUTTON2)
            I_LastButton=MIDDLE_BUTTON;
         else if(diff&BUTTON3)
            I_LastButton=LEFT_BUTTON;
         else
            I_LastButton=0;
         diff=EV_BUTTONS(ev)&~buttons;
         buttons=EV_BUTTONS(ev);
         if(diff)
            return(M_BUTTON);
         else
            return(M_RBUTTON);
      }
      if(EV_TAG(ev)&T_REL_LOCATOR)
      {
         if(EV_DX(ev)>MouseRX)
            EV_DX(ev)<<=1;
         if(EV_DY(ev)>MouseRY)
            EV_DY(ev)<<=1;
         MouseDX+=EV_DX(ev);
         MouseDY-=EV_DY(ev);
         if(MouseDX/MouseRX!=0 || MouseDY/MouseRY!=0)
         {
            I_MouseX+=MouseDX/MouseRX;
            I_MouseY+=MouseDY/MouseRY;
            MouseDX%=MouseRX;
            MouseDY%=MouseRY;
            return(M_MOVE);
         }
         else
            continue;
      }
      return(K_OTHER);
   }
/*NOTREACHED*/
}

int   ConsoleKeyPressed(void)
{
   return(ev_count()>0);
}

void  ConsoleCloseInterface(void)
{
   ConsoleClearScreen();
   ev_close();
   ioctl(0,TCSETAF,&oldterm);
}

void  ConsoleUpdateCell(unsigned x,unsigned y,unsigned offs,ScreenCell *cell)
{
   unsigned char  attr=0x07;

   if(cell->flags&COLOR)
      attr=cell->foreground|(cell->background<<4);
   if(cell->flags&REVERSE)
      attr=(attr>>4)|(attr<<4);
   if(cell->flags&BOLD)
      attr|=0x08;
   if(cell->flags&BLINK)
      attr|=0x80;

   offs<<=1;
   VideoMem[offs]=(char)(cell->ch);
   VideoMem[offs+1]=attr;
}

void  SCOConsoleInit()
{
   dmask_t  dmask;
   struct   vt_mode  smode;
   struct   vid_info vinfo;
   struct   m6845_info  minfo;
   int   i;

   close(1);
   dup(0);

   I_ScreenFlags=CONSOLE;

   switch(UserVideoMode=ioctl(0,CONS_GET,0))
   {
      case(M_C40x25):
      case(M_C80x25):
      case(M_ENH_C80x43):
         I_ScreenFlags|=HAS_COLORS;
      case(M_B40x25):
      case(M_B80x25):
      case(M_ENH_B80x43):
         font=8;
         break;
      case(M_ENH_C40x25):
      case(M_ENH_C80x25):
         I_ScreenFlags|=HAS_COLORS;
      case(M_EGAMONO80x25):
      case(M_ENH_B40x25):
      case(M_ENH_B80x25):
      case(M_MCA_MODE):
         font=14;
         break;
      case(M_VGA_40x25):
      case(M_VGA_80x25):
         I_ScreenFlags|=HAS_COLORS;
      case(M_VGA_M80x25):
         font=16;
         break;
      default:
         fprintf(stderr,"\n\rError: Unknown or graphics screen mode\n\r");
         sleep(1);
         UserVideoMode=M_VGA_80x25;
         ioctl(0,MODESWITCH|UserVideoMode,0);
   }

   ProgramVideoMode=UserVideoMode;

   MouseRY=font;
   MouseRX=8;

   ioctl(0,TCGETA,&oldterm);
   newterm=oldterm;
   newterm.c_lflag=0;
   newterm.c_iflag=0;
   newterm.c_cc[VMIN]=1;
   newterm.c_cc[VTIME]=0;
   ioctl(0,TCSETAF,&newterm);

   ev_init();
   dmask=D_STRING|D_REL;   /* receive keyboard and mouse events */
   qfd=ev_open(&dmask);
   if(qfd<0)
   {
      fprintf(stderr,"\r\nError: Cannot open event queue\r\n");
      ioctl(0,TCSETAF,&oldterm);
      abort();
   }

   ioctl(0,GIO_STRMAP,strmap);

   vinfo.size=sizeof(vinfo);
   ioctl(0,CONS_GETINFO,&vinfo);

   I_ScreenWidth=vinfo.mv_csz;
   I_ScreenHeight=vinfo.mv_rsz;
   I_ScreenSize=I_ScreenHeight*I_ScreenWidth;
   _I_Screen=(ScreenCell*)calloc(I_ScreenSize,sizeof(*_I_Screen));
   VideoSave=(char*)calloc(I_ScreenSize,2);
   PVideoMem=(char*)ioctl(0,MAPCONS,NULL);
   minfo.size=sizeof(minfo);
   minfo.screen_top=0;
   ioctl(0,CONS_6845INFO,&minfo);
   if(vinfo.m_num!=ioctl(0,CONSADP,ioctl(0,CONS_CURRENT,0)))
      VideoMem=VideoSave;
   else
      VideoMem=PVideoMem+minfo.screen_top*2;

   signal(SIG_REL, rel_screen);
   signal(SIG_ACQ, acq_screen);
/*
* Set up the data structure that asks the driver
* to send you signals when the screens are switched.
* mode == VT_PROCESS means send screen switch signals.
* mode == VT_AUTO means turn off screen switch signals (regular mode).
* relsig == the signal you want when the user switches away.
* acqsig == the signal you want when the user switches back to you.
*/
   smode.mode = VT_PROCESS;
   smode.waitv = 0;    /* not implemented, reserved */
   smode.relsig = SIG_REL;
   smode.acqsig = SIG_ACQ;
   smode.frsig  = SIGINT;  /* not implemented, reserved */
   ioctl(0, VT_SETMODE,&smode);

   for(i=0; i<I_ScreenSize; i++)
   {
      VideoMem[i<<1]=' ';
      VideoMem[(i<<1)+1]=7;
   }

   if(I_ScreenFlags&HAS_COLORS)
   {
      short i;
      I_ColorsNum=8;
      CheckPtr(_I_ColorMap=(Rgb*)calloc(I_ColorsNum,sizeof(Rgb)));
      CheckPtr(_I_ColorInfo=(RgbState*)calloc(I_ColorsNum,sizeof(*_I_ColorInfo)));
      for(i=0; i<I_ColorsNum; i++)
      {
         _I_ColorMap[i].r=((i>>2)&1)*0xAA;
         _I_ColorMap[i].g=((i>>1)&1)*0xAA;
         _I_ColorMap[i].b=(i&1)*0xAA;
      }
   }
   if(dmask&D_REL)
   {
      I_ScreenFlags|=HAS_MOUSE;
      I_MouseX=I_ScreenWidth>>1;
      I_MouseY=I_ScreenHeight>>1;
   }

   _UpdateCell=ConsoleUpdateCell;
/* _DefineColor=ConsoleDefineColor;*/
   _CloseInterface=ConsoleCloseInterface;
   _KeyPressed=ConsoleKeyPressed;
   _ReadKey=ConsoleReadKey;
   _SetCursor=ConsoleSetCursor;
   _Sync=ConsoleSync;
   _Suspend=ConsoleSuspend;
   _Resume=ConsoleResume;
   _Redraw=NULL;  /* has no redraw ability */
   _Bell=ConsoleBell;
}

/*
* this is the signal handler for when the user screen flips
* away from us.
*/
void
rel_screen()
{
   signal(SIG_REL, rel_screen);
   memcpy(VideoSave,VideoMem,I_ScreenSize<<1);
   VideoMem=VideoSave;
   ioctl(0, VT_RELDISP, VT_TRUE);
}

/*
* this is the signal handler for when the user screen flips
* back to us.
*/
void
acq_screen()
{
   struct   m6845_info  minfo;
   signal(SIG_ACQ, acq_screen);
   minfo.size=sizeof(minfo);
   ioctl(0,CONS_6845INFO,&minfo);
   VideoMem=PVideoMem+minfo.screen_top*2;
   memcpy(VideoMem,VideoSave,I_ScreenSize<<1);
   ioctl(0, VT_RELDISP, VT_ACKACQ);
}
