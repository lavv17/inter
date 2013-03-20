/*
** File:    ow.h
*/
#ifndef  _OW_INCLUDED
#define  _OW_INCLUDED
#include "keys.h"

#define  EV_QUEUE_SIZE  16

#define RIGHT   0x4000
#define DOWN    0x4000
#define MIDDLE  0x2000
#define LEFT    0x0000
#define UP      0x0000

#define OKAY    0
#define ERROR   -1
#define CANCEL  -2

#define OPEN    1

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

typedef unsigned char   byte;
typedef signed char     sbyte;

typedef struct limits
{
        byte    Left;
        byte    Top;
        byte    Right;
        byte    Bottom;
}
        Limits;

typedef struct objentry
{
        int     (*create)(void**ptr,void*param);
        int     (*destroy)(void*ptr);
        int     (*display)(void*ptr);
        int     (*close)(void*ptr);     /* It is called when the window */
                                        /* is to be close               */
        int     (*activate)(void*ptr,int key);
        int     (*chosen)(void*ptr,int flag);
                                        /* It tells the object that
                                           user chosen OKAY or CANCEL   */
        int     (*getlimits)(void*ptr,Limits*info);
        int     (*memchanged)(void*ptr,void*mem);
                                        /* It tells the object that
                                           memory at an address was
                                           changed                      */
}
        ObjEntry;

typedef struct objchain
{
        ObjEntry        *Entry;
        void            *ObjInfo;
        struct objchain *Next;
}
        ObjChain;

typedef struct window
{
   int             X,Y;
   unsigned        Width,Height;
   Cell            *Palette;
   struct window   *NextOpen,*PrevOpen,*MasterWindow,*SubWindow;
   Cell            *OldText;
   byte            Flags;
}
   Window;

typedef struct event
{
   unsigned code;
   Window   *wnd;
   int      button,buttons_state;
   int      s_mouse_x,s_mouse_y;
   int      w_mouse_x,w_mouse_y;
   unsigned long time;
}
   Event;

extern  Window  *Root,          /* The root of window                   */
                *Used,          /* The using window                     */
                *Lower,         /* The background window(usually Screen)*/
                *Upper,
                *Focus;

void  OwDisplay(Window*);
void  OwClose(void);
void  OwCloseAll(void);
void  OwCreate(Window **win,int x,int y,unsigned w,unsigned h,Cell *palette);
void  OwCreateArea(Window **win,int x,int y,unsigned w,unsigned h,Cell *palette,Window *master);
void  OwInitialize(Cell *palette);
void  OwExit(void);
void  OwDestroy(Window *w);
void  OwUse(Window *w);
void  OwMove(int x,int y);
void  OwMoveResize(int x,int y,unsigned w,unsigned h);
void  OwResize(unsigned w,unsigned h);
void  OwFocus(Window *w);

void  OwSetCell(int x,int y,Cell *c);
void  OwGetCell(int x,int y,Cell *c);
void  OwSetCursorPos(int x,int y);
void  OwCopyArea(int x,int y,unsigned w,unsigned h,int newx,int newy,int clear_flag);
void  OwPutString(int x,int y,unsigned char *s,Cell *a);
void  OwPutFormattedString(int x,int y,unsigned char *s,Cell *a,...);
void  OwDrawFrame(int x,int y,unsigned w,unsigned h,unsigned char*f,Cell *a);
void  OwDrawShadow(int x,int y,unsigned w,unsigned h,Cell *a);
void  OwClearBox(int x,int y,unsigned w,unsigned h,Cell *c);
void  OwClear(void);

int   OwWaitEvent(Event *ev,int timeout);
int   OwSimpleGetString(int x,int y,int width,unsigned char*buffer,
                        int maxlength,Cell *c);

void  OwAbsolute(int*x,int size,int field);

#define OwIsOpen(w)     ((w)->Flags&OPEN)
#define UW              (Used->Width)
#define UH              (Used->Height)
#define UX              (Used->X)
#define UY              (Used->Y)
#define UF              (Used->Flags)
#define UPAL            (Used->Palette)

/* ??? */
enum    attributes
{
        NORMAL=0,       /* normal text          */
        BNORMAL,        /* bright normal text   */
        _REVERSE,        /* reverse text         */
        BREVERSE,       /* bright reverse text  */
        BUTTON,         /* button attribute     */
        BBUTTON,        /* bright button letter */
        FRAME,          /* frame attribute      */
        SHADOW,         /* shadow attribute     */
        INPUT           /* input field attribute */
};

        /* some useful macroses */
#define        New(s)          (s=CheckPtr(malloc(sizeof(*s))))
#define        FULL            0,0,UW,UH
#define        FULLB(n)        n,n,UW-2*n,UH-2*n
#define        FULL1           1,1,UW-2,UH-2
#define        SINGLE          "ÚÄ¿³³ÀÄÙ"
#define        DOUBLE          "ÉÍ»ººÈÍ¼"

#endif
