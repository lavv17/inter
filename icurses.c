/*____________________________________________________________________________
**
** File:          icurses.c
**
** Description:   Curses interface
**____________________________________________________________________________
*/

#include <curses.h>
#include <poll.h>
#include <malloc.h>
#include "inter.h"
#include "check.h"

unsigned FuncKeysNum=12;

struct   keytab
{
   int      curses_key;
   unsigned key;
};

static   struct   keytab   KeyTable[]=
{
   {  KEY_LEFT,   K_LEFT      }, {  KEY_UP,     K_UP        },
   {  KEY_RIGHT,  K_RIGHT     }, {  KEY_DOWN,   K_DOWN      },
   {  KEY_HOME,   K_HOME      }, {  KEY_END,    K_END       },
   {  KEY_NPAGE,  K_PGDN      }, {  KEY_PPAGE,  K_PGUP      },
   {  KEY_DC,     K_DEL       }, {  KEY_IC,     K_INS       },
   {  KEY_SLEFT,  K_CTRL_LEFT }, {  KEY_SRIGHT, K_CTRL_RIGHT},
   {  KEY_SHOME,  K_CTRL_HOME }, {  KEY_SEND,   K_CTRL_END  },
   {  KEY_ENTER,  K_ENTER     }, {  KEY_EXIT,   K_ESC       },
   {  KEY_BACKSPACE,K_CTRL_H  }, {  KEY_CANCEL, K_ESC       },
   {  -1 }
};
static   chtype   SymbolTable[256];

static void InitSymbolTable(void)
{
   int   c;

   for(c=0; c<256; c++)
      SymbolTable[c]=c;

   /* corners */
   SymbolTable[0xC9]=SymbolTable[0xD5]=SymbolTable[0xDA]=SymbolTable[0xD6]=ACS_ULCORNER;
   SymbolTable[0xB7]=SymbolTable[0xB8]=SymbolTable[0xBB]=SymbolTable[0xBF]=ACS_URCORNER;
   SymbolTable[0xC0]=SymbolTable[0xC8]=SymbolTable[0xD3]=SymbolTable[0xD4]=ACS_LLCORNER;
   SymbolTable[0xBC]=SymbolTable[0xBD]=SymbolTable[0xBE]=SymbolTable[0xD9]=ACS_LRCORNER;

   /* tees */
   SymbolTable[0xC2]=SymbolTable[0xCB]=SymbolTable[0xD1]=SymbolTable[0xD2]=ACS_TTEE;
   SymbolTable[0xB4]=SymbolTable[0xB5]=SymbolTable[0xB6]=SymbolTable[0xB9]=ACS_RTEE;
   SymbolTable[0xC1]=SymbolTable[0xCA]=SymbolTable[0xCF]=SymbolTable[0xD0]=ACS_BTEE;
   SymbolTable[0xC3]=SymbolTable[0xC6]=SymbolTable[0xC7]=SymbolTable[0xCC]=ACS_LTEE;

   /* lines */
   SymbolTable[0xB3]=SymbolTable[0xBA]=ACS_VLINE;
   SymbolTable[0xC4]=SymbolTable[0xCD]=ACS_HLINE;

   /* plus */
   SymbolTable[0xC5]=SymbolTable[0xCE]=SymbolTable[0xD7]=SymbolTable[0xD8]=ACS_PLUS;

   /* checker board (stipple) */
   SymbolTable[0xB0]=SymbolTable[0xB1]=SymbolTable[0xB2]=ACS_CKBOARD;

   /* plus/minus sign */
   SymbolTable[0xF1]=ACS_PLMINUS;

   /* degree sign */
   SymbolTable[0xF8]=ACS_DEGREE;

   /* bullet */
   SymbolTable[0xF9]=SymbolTable[0xFA]=ACS_BULLET;

   /* diamond */
   SymbolTable[0x03]=ACS_DIAMOND;
}

void  CursesSync(void)
{
   refresh();
}

void  CursesRedraw(void)
{
   clearok(stdscr,TRUE);
   refresh();
}

void  CursesBell(void)
{
   beep();
}

unsigned CursesReadKey(int timeout)
{
   int   key;
   struct   keytab   *ScanKeys;

   if(timeout<0)
      nodelay(stdscr,FALSE);
   else if(timeout==0)
      nodelay(stdscr,TRUE);
   else      
   {
      struct pollfd  pfd;
      pfd.fd=0;
      pfd.events=POLLIN;
      if(poll(&pfd,1,timeout)<1)
         return(K_NONE);
      if(pfd.revents&POLLIN==0)
         return(K_NONE);
/*      if((timeout+50)/100==0)
         nodelay(stdscr,TRUE);
      else
         halfdelay((timeout+50)/100);*/
   }
   key=getch();
   if(key==ERR)
      return(K_NONE);
   if(key==KEY_REFRESH)
   {
      CursesRedraw();
      return(K_OTHER);
   }
   if(key!=ERR && key>255)
   {
      if(key>=KEY_F(FuncKeysNum*0+1) && key<=KEY_F(FuncKeysNum*1))
          return(      K_F(key-KEY_F(FuncKeysNum*0)));
      if(key>=KEY_F(FuncKeysNum*1+1) && key<=KEY_F(FuncKeysNum*2))
          return(K_Shift_F(key-KEY_F(FuncKeysNum*1)));
      if(key>=KEY_F(FuncKeysNum*2+1) && key<=KEY_F(FuncKeysNum*3))
          return( K_Ctrl_F(key-KEY_F(FuncKeysNum*2)));
      if(key>=KEY_F(FuncKeysNum*3+1) && key<=KEY_F(FuncKeysNum*4))
          return(  K_Alt_F(key-KEY_F(FuncKeysNum*3)));

      for(ScanKeys=KeyTable; ScanKeys->curses_key!=-1; ScanKeys++)
         if(key==ScanKeys->curses_key)
            return(ScanKeys->key);
      return(K_OTHER);
   }
   return((unsigned)key);
}

int   CursesKeyPressed(void)
{
   int   key;

   nodelay(stdscr,TRUE);
   key=getch();
   nodelay(stdscr,FALSE);
   if(key==ERR)
      return(FALSE);
   ungetch(key);
   return(TRUE);
}

void  CursesUpdateCell(unsigned x,unsigned y,unsigned offs,ScreenCell *cell)
{
   unsigned long  attr;
   unsigned       ch;

   attr=0;
   ch=cell->ch;

   if(cell->flags&REVERSE)
      attr|=A_REVERSE;
   if(cell->flags&BOLD)
      attr|=A_BOLD;
   if(cell->flags&UNDERLINE)
      attr|=A_UNDERLINE;
   if(cell->flags&DIM)
      attr|=A_DIM;
   if(cell->flags&BLINK)
      attr|=A_BLINK;
   if(cell->flags&COLOR)
      attr|=COLOR_PAIR(cell->pair+1);

   if(ch&256)
   {
      attr|=A_ALTCHARSET;
   }
   else
   {
      if(cell->flags&GRAPHIC)
      {
         attr|=SymbolTable[ch]&A_ATTRIBUTES;
         ch=SymbolTable[ch]&A_CHARTEXT;
      }
   }

   attrset(attr);
   mvaddch(y,x,ch&255);
}

void  CursesDefinePair(unsigned pair,unsigned f,unsigned b)
{
   init_pair((short)(pair+1),(short)f,(short)b);
}

void  CursesSetCursor(unsigned x,unsigned y,int type)
{
   move((short)y,(short)x);
   curs_set(type);
}

void  CursesSuspend(void)
{
   endwin();
}

void  CursesResume(void)
{
   clearok(stdscr,TRUE);
   refresh();
}

void  CursesInit(void)
{
   short i;
   ScreenCell  blank={0,' '};

   initscr();
   start_color();
   noecho();
   nonl();
   keypad(stdscr,TRUE);
   intrflush(stdscr,FALSE);
   raw();

   refresh();

   InitSymbolTable();

   I_ScreenFlags=0;

   if(has_colors())
   {
      I_ScreenFlags|=HAS_COLORS|HAS_PAIRS;
      if(can_change_color())
         I_ScreenFlags|=CAN_CHANGE_COLOR;
   }

   I_ColorsNum=COLORS;
   I_ScreenWidth=COLS;
   I_ScreenHeight=LINES;
   I_PairsNum=COLOR_PAIRS-1;
   I_ScreenSize=I_ScreenWidth*I_ScreenHeight;
   CheckPtr(_I_Screen=(ScreenCell*)calloc(I_ScreenSize,sizeof(*_I_Screen)));
   for(i=0; i<I_ScreenSize; i++)
      _I_Screen[i]=blank;

   if(I_ScreenFlags&HAS_COLORS)
   {
      short r,g,b,f;

      CheckPtr(_I_ColorMap=(Rgb*)calloc(I_ColorsNum,sizeof(Rgb)));
      CheckPtr(_I_ColorInfo=(RgbState*)calloc(I_ColorsNum,sizeof(*_I_ColorInfo)));
      CheckPtr(_I_PairMap=(Pair*)calloc(I_PairsNum,sizeof(*_I_PairMap)));
      CheckPtr(_I_PairInfo=(PairState*)calloc(I_PairsNum,sizeof(*_I_PairInfo)));
      for(i=0; i<I_ColorsNum; i++)
      {
         if(I_ScreenFlags&CAN_CHANGE_COLOR)
         {
            color_content(i,&r,&g,&b);
            _I_ColorMap[i].r=(unsigned char)(((unsigned long)r<<8)/1000);
            _I_ColorMap[i].g=(unsigned char)(((unsigned long)g<<8)/1000);
            _I_ColorMap[i].b=(unsigned char)(((unsigned long)b<<8)/1000);
            if(i==0 || i==I_ColorsNum-1)
                _I_ColorInfo[i].flags|=READONLY;
         }
         else
         {
            _I_ColorMap[i].b=((i>>2)&1)*0xAA;
            _I_ColorMap[i].g=((i>>1)&1)*0xAA;
            _I_ColorMap[i].r=(i&1)*0xAA;
         }
      }
      for(i=0; i<I_PairsNum; i++)
      {
         pair_content(i,&f,&b);
         _I_PairMap[i].foreground=f;
         _I_PairMap[i].background=b;
      }
   }

   _UpdateCell=CursesUpdateCell;
/* _DefineColor=CursesDefineColor;*/
   _DefinePair=CursesDefinePair;
   _CloseInterface=endwin;
   _KeyPressed=CursesKeyPressed;
   _ReadKey=CursesReadKey;
   _SetCursor=CursesSetCursor;
   _Sync=CursesSync;
   _Suspend=CursesSuspend;
   _Resume=CursesResume;
   _Redraw=CursesRedraw;
   _Bell=CursesBell;
}
