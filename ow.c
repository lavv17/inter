/*____________________________________________________________________________
**
**      File:  ow.c
**____________________________________________________________________________
*/

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "inter.h"
#include "ow.h"
#include "check.h"

Window   *Root;          /* The root window                      */
Window   *Used;          /* The window being used                */
Window   *Lower;
Window   *Upper;
Window   *Focus;

Event    EventQueue[EV_QUEUE_SIZE];
int      EventQueueHead;
int      EventQueueTail;

int      sx,sy;

static   Cell  *OwGetCellAddr(int x,int y);

/*____________________________________________________________________________
*/
void  OwInitialize(Cell *c)
{
   Window   *new;
/*   OpenInterface();*/
   OwCreate(&new,0,0,I_ScreenWidth,I_ScreenHeight,c);
/*   OwDisplay(new);*/
   Root=new;
   EventQueueTail=EventQueueHead=0;
}

void  OwExit(void)
{
   OwCloseAll();
/*   CloseInterface();*/
}

void  OwUse(Window *w)
{
   Used=w;
}
void  OwFocus(Window *w)
{
   Focus=w;
}
void  OwSetPalette(Cell *p)
{
   UPAL=p;
}

int   OwWaitEvent(Event *ev,int timeout)
{
   Window   *wnd;

   if(EventQueueHead==EventQueueTail)
   {
      WaitKey(timeout);
      if(I_LastKey==K_NONE)
         return(0);
      ev->code=I_LastKey;
      ev->s_mouse_x=I_MouseX;
      ev->s_mouse_y=I_MouseY;
      ev->button=I_LastButton;
      ev->buttons_state=I_Buttons;
      ev->time=I_LastTime;
      if(IsMouseEvent(I_LastKey))
      {
         for(wnd=Upper; wnd; wnd=wnd->PrevOpen)
         {
            if((unsigned)(I_MouseX-wnd->X)<wnd->Width
            && (unsigned)(I_MouseY-wnd->Y)<wnd->Height)
            {
               ev->wnd=wnd;
               ev->w_mouse_x=I_MouseX-wnd->X;
               ev->w_mouse_y=I_MouseY-wnd->Y;
               for(wnd=wnd->SubWindow; wnd; wnd=wnd->SubWindow)
               {
                  if((unsigned)(ev->w_mouse_x-wnd->X)<wnd->Width
                  && (unsigned)(ev->w_mouse_y-wnd->Y)<wnd->Height)
                  {
                     ev->wnd=wnd;
                     ev->w_mouse_x=ev->w_mouse_x-wnd->X;
                     ev->w_mouse_y=ev->w_mouse_y-wnd->Y;
                     break;
                  }
               }
               break;
            }
         }
         if(wnd==NULL)
         {
            ev->wnd=NULL;
            ev->w_mouse_x=I_MouseX;
            ev->w_mouse_y=I_MouseY;
         }        
      }
   }
   else
   {
      *ev=EventQueue[EventQueueTail];
      EventQueueTail++;
      if(EventQueueTail==EV_QUEUE_SIZE)
         EventQueueTail=0;
   }
   if(IsKeyboardEvent(ev->code))
      ev->wnd=Focus;
   return(1);
}

static
void  OwCorrectGeometry(int *x,int *y,unsigned *w,unsigned *h,int sx,int sy,unsigned sw,unsigned sh)
{
   if(*w>sw)            /* correct parameters */
      *w=sw;
   if(*h>sh)
      *h=sh;

   OwAbsolute(x,*w,sw);
   OwAbsolute(y,*h,sh);

   if(*x<sx)
      *x=sx;
   if(*y<sy)
      *y=sy;

   if(*x+*w>sw)
      *x=sw-*w;
   if(*y+*h>sw)
      *y=sw-*h;
}

void  OwCreate(Window **wnd,int x,int y,unsigned w,unsigned h,Cell *palette)
{
   OwCorrectGeometry(&x,&y,&w,&h,0,0,I_ScreenWidth,I_ScreenHeight);

   New(*wnd);
   Used=*wnd;
   Used->X=x;
   Used->Y=y;
   Used->Width=w;
   Used->Height=h;
   Used->Palette=palette;
   Used->Flags=0;
   Used->MasterWindow=NULL;
   Used->SubWindow=NULL;
}

void  OwCreateArea(Window **wnd,int x,int y,unsigned w,unsigned h,Cell *palette,Window *master)
{
   OwCorrectGeometry(&x,&y,&w,&h,0,0,master->Width,master->Height);

   if(master->MasterWindow)
      master=master->MasterWindow;

   New(*wnd);
   Used=*wnd;
   Used->X=x;
   Used->Y=y;
   Used->Width=w;
   Used->Height=h;
   Used->Palette=palette;
   Used->Flags=0;
   Used->MasterWindow=master;
   Used->SubWindow=master->SubWindow;
   master->SubWindow=Used;
}

void  OwDestroy(Window *wnd)
{
   Window   *sw,*sws;
   if(wnd->MasterWindow)
      wnd=wnd->MasterWindow;
   if(wnd->Flags&OPEN)
   {
      OwUse(wnd);
      OwClose();
   }
   for(sw=wnd->SubWindow; sw; )
   {
      sws=sw->SubWindow;
      free(sw);
      sw=sws;
   }
   free(wnd);
}

void  OwMove(int x,int y)
{
   int   startx,starty;
   int   endx,endy;
   int   stepx,stepy;
   int   shiftx,shifty;
   Cell  *ptr;
   Cell  save;
   Cell  *old;

   if(Used->MasterWindow)
      return;

   OwCorrectGeometry(&x,&y,&UW,&UH,0,0,I_ScreenWidth,I_ScreenHeight);

   shiftx=x-UX;
   shifty=y-UY;

   if(shiftx==0 && shifty==0)
      return;

   if(Used->Flags&OPEN)
   {
      if(shiftx>0)
      {
         startx=UW-1;
         stepx=-1;
         endx=-1;
      }
      else
      {
         startx=0;
         stepx=1;
         endx=UW;
      }
      if(y>UY)
      {
         starty=UH-1;
         stepy=-1;
         endy=-1;
      }
      else
      {
         starty=0;
         stepy=1;
         endy=UH;
      }

      for(y=starty; y!=endy; y+=stepy)
      {
         old=Used->OldText+y*Used->Width+startx;
         for(x=startx; x!=endx; x+=stepx,old+=stepx)
         {
            ptr=OwGetCellAddr(x,y);
            if(ptr!=NULL)
            {
               save=*ptr;
               *ptr=*old;
            }
            else
            {
               GetScreenCell(sx,sy,&save);
               SetScreenCell(sx,sy,old);
            }
            ptr=OwGetCellAddr(x+shiftx,y+shifty);
            if(ptr!=NULL)
            {
               *old=*ptr;
               *ptr=save;
            }
            else
            {
               GetScreenCell(sx,sy,old);
               SetScreenCell(sx,sy,&save);
            }
         }
      }
   }

   UX+=shiftx;
   UY+=shifty;
}

void  OwCopyArea(int x,int y,unsigned w,unsigned h,int newx,int newy,int clear)
{
   int   startx,starty;
   int   endx,endy;
   int   stepx,stepy;
   int   shiftx,shifty;
   Cell  buf,*ptr;

   if(w<1 || h<1 || (x==newx && y==newy))
      return;
   if(newx<0)
   {
      if(w<=-newx)
         return;
      x-=newx;
      w+=newx;
      newx=0;
   }
   if(newy<0)
   {
      if(h<=-newy)
         return;
      y-=newy;
      h+=newy;
   }
   if(newx+w>UW)
   {
      if(newx>=UW)
         return;
      w=UW-newx;
   }
   if(newy+h>UH)
   {
      if(newy>=UH)
         return;
      h=UH-newy;
   }

   shiftx=newx-x;
   shifty=newy-y;

   if(x<newx)
   {
      startx=x+w-1;
      endx=-1;
      stepx=-1;
   }
   else
   {
      startx=x;
      endx=x+w;
      stepx=1;
   }
   if(y<newy)
   {
      starty=y+h-1;
      endy=-1;
      stepy=-1;
   }
   else
   {
      starty=y;
      endy=y+h;
      stepy=1;
   }

   for(y=starty; y!=endy; y+=stepy)
   {
      for(x=startx; x!=endx; x+=stepx)
      {
         if(x+shiftx<0 || y+shifty<0 || x+shiftx>=UW || y+shifty>=UH)
            continue;
         if(x<0 || y<0 || x>=UW || y>=UH)
            OwSetCell(x+shiftx,y+shifty,UPAL+0);
         else
         {
            ptr=OwGetCellAddr(x,y);
            if(ptr==NULL)
            {
               GetScreenCell(sx,sy,&buf);
               if(clear)
                  SetScreenCell(sx,sy,UPAL+0);
               OwSetCell(x+shiftx,y+shifty,&buf);
            }
            else
            {
               OwSetCell(x+shiftx,y+shifty,ptr);
               if(clear)
                  *ptr=UPAL[0];
            }
         }
      }
   }
}

void  OwMoveResize(int x,int y,unsigned w,unsigned h)
{
   Window   *wnd=Used;
   int      was_open=Used->Flags&OPEN;

   if(Used->MasterWindow)
      return;

   OwCorrectGeometry(&x,&y,&w,&h,0,0,I_ScreenWidth,I_ScreenHeight);

   if(w==wnd->Width && h==wnd->Height)
   {
      OwMove(x,y);
      return;
   }
   OwClose();
   wnd->X=x;
   wnd->Y=y;
   wnd->Width=w;
   wnd->Height=h;
   if(was_open)
      OwDisplay(wnd);
}

void  OwResize(unsigned w,unsigned h)
{
   OwMoveResize(UX,UY,w,h);
}

Cell  *OwGetCellAddr(x,y)
int   x,y;
{
   register Window   *wnd;

   sx=x+Used->X;
   sy=y+Used->Y;
   if(Used->MasterWindow!=NULL)
   {
      wnd=Used->MasterWindow;
      sx+=wnd->X;
      sy+=wnd->Y;
   }
   else
      wnd=Used;
   for(wnd=wnd->NextOpen; wnd!=NULL; wnd=wnd->NextOpen)
   {
      if((unsigned)(sx-wnd->X)<wnd->Width
      && (unsigned)(sy-wnd->Y)<wnd->Height)
         return(wnd->OldText+(wnd->Width*(sy-wnd->Y)+sx-wnd->X));
   }
   return(NULL);
}

void    OwDisplay(Window *wnd)
{
   int            ww,wh;
   register int   x,y;

   ww=wnd->Width;
   wh=wnd->Height;

   CursorType(C_INVISIBLE);

   if(Used && !(Used->Flags&OPEN))
      OwUse(Upper);

   if(wnd->MasterWindow)
      wnd=wnd->MasterWindow;

   if(!(wnd->Flags&OPEN))
   {
      Cell  *save;
      Cell  *ptr;

      if(Used!=NULL)
      {
         wnd->NextOpen=Used->NextOpen;
         if(Used->NextOpen!=NULL)
            Used->NextOpen->PrevOpen=wnd;
         Used->NextOpen=wnd;  /* insert the window    */
      }
      else
      {
         if(Lower!=NULL)
            Lower->PrevOpen=wnd;
         wnd->NextOpen=Lower;
         Lower=wnd;
      }
      wnd->PrevOpen=Used;
      if(wnd->NextOpen==NULL)
         Upper=wnd;
      Used=wnd;
      save=wnd->OldText=CheckPtr(malloc(ww*wh*sizeof(Cell)));
      for(y=0; y<wh; y++)
      {
         for(x=0; x<ww; x++)
            if((ptr=OwGetCellAddr(x,y))!=NULL)
            {
               *(save++)=*ptr;
               *(ptr++)=wnd->Palette[0];
            }
            else
            {
               GetScreenCell(sx,sy,save++);
               SetScreenCell(sx,sy,&wnd->Palette[0]);
            }
      }
      wnd->Flags|=OPEN;
      for(OwUse(wnd->SubWindow); Used; OwUse(Used->SubWindow))
         OwClearBox(FULL,UPAL+0);
      OwUse(wnd);
   }
   else
   {
      Window  *wnd1;
      Window  *wnd2;
      Window  *NextOfUsing;
      Cell    *ptr;
      Cell    *ptr1;
      Cell    save;

      NextOfUsing=Used?Used->NextOpen:Lower;

      if(Used==wnd || NextOfUsing==wnd)
         return;

      for(y=0; y<wh; y++)
      {
         for(x=0; x<ww; x++)
         {
            sx=x+wnd->X;
            sy=y+wnd->Y;

            for(wnd1=wnd->NextOpen; wnd1 && wnd1 != NextOfUsing &&
                    ((unsigned)(sx - wnd1->X) >= wnd1->Width ||
                     (unsigned)(sy - wnd1->Y) >= wnd1->Height);
                            wnd1=wnd1->NextOpen);
            for(wnd2=NextOfUsing; wnd2 && wnd2 != wnd &&
                    ((unsigned)(sx - wnd2->X) >= wnd2->Width ||
                     (unsigned)(sy - wnd2->Y) >= wnd2->Height);
                            wnd2=wnd2->NextOpen);
            if(wnd1!=NextOfUsing && wnd2!=wnd)
            {
               ptr=&wnd->OldText[y*wnd->Width+x];
               if(wnd1)
               {
                  ptr1=wnd1->OldText
                     +(sy-wnd1->Y)*wnd1->Width+(sx-wnd1->X);
                  save=*ptr1;
                  *ptr1=*ptr;
               }
               else
               {
                  GetScreenCell(sx,sy,&save);
                  SetScreenCell(sx,sy,ptr);
               }
               if(wnd2)
               {
                  ptr1=wnd2->OldText
                     +((sy-wnd2->Y)*wnd2->Width+(sx-wnd2->X))*2;
                  *ptr=*ptr1;
                  *ptr1=save;
               }
               else
               {
                  GetScreenCell(sx,sy,ptr);
                  SetScreenCell(sx,sy,&save);
               }
            }
         }
      }
      if(wnd->PrevOpen)
         wnd->PrevOpen->NextOpen=wnd->NextOpen;
      else
         Lower=wnd->NextOpen;
      if(wnd->NextOpen)
         wnd->NextOpen->PrevOpen=wnd->PrevOpen;
      else
         Upper=wnd->PrevOpen;
      if(Used!=NULL)
      {
         wnd->NextOpen=Used->NextOpen;
         if(Used->NextOpen!=NULL)
            Used->NextOpen->PrevOpen=wnd;
         Used->NextOpen=wnd;  /* insert the window    */
      }
      else
      {
         if(Lower!=NULL)
            Lower->PrevOpen=wnd;
         wnd->NextOpen=Lower;
         Lower=wnd;
      }
      wnd->PrevOpen=Used;
      if(wnd->NextOpen==NULL)
              Upper=wnd;
      OwUse(wnd);
   }
}

void    OwClose()
{
   int             ww;
   int             wh;
   int             x,y;
   Cell            *addr;
   Cell            *save;

   if(!Used || !(Used->Flags&OPEN))
      return;

   if(Used->MasterWindow)
      Used=Used->MasterWindow;

   ww=Used->Width;
   wh=Used->Height;

   CursorType(C_INVISIBLE);

   save=Used->OldText;
   for(y=0; y<wh; y++)
   {
      for(x=0; x<ww; x++)
         if((addr=OwGetCellAddr(x,y))!=NULL)
            *(addr++)=*(save++);
         else
            SetScreenCell(sx,sy,save++);
   }
   free(Used->OldText);
   if(Used->PrevOpen)
      Used->PrevOpen->NextOpen=Used->NextOpen;
   else
      Lower=Used->NextOpen;
   if(Used->NextOpen)
      Used->NextOpen->PrevOpen=Used->PrevOpen;
   else
      Upper=Used->PrevOpen;
   Used->Flags&=~OPEN;
   Used=Used->PrevOpen;
}

void  OwCloseAll(void)
{
   OwUse(Upper);
   while(Used)
      OwClose();
}

void  OwAbsolute(x,size,field)
int   *x,size,field;
{
   if(*x>0)        /* It is supposed that MIDDLE<RIGHT=DOWN        */
   {
      if(((*x&(MIDDLE>>1)) && (*x&(RIGHT>>1)))
      || (!(*x&MIDDLE) && (*x&RIGHT)))
         /* X from RIGHT/DOWN    */
         *x+=-RIGHT+field-size;
      else if(*x&(MIDDLE>>1) || (*x&MIDDLE))
         /* X from MIDDLE        */
         *x+=-MIDDLE+(field-size)/2;
   }
}

void  OwSetCell(int x,int y,Cell *c)
{
   if((unsigned)x<Used->Width && (unsigned)y<Used->Height)
   {
      Cell *addr;
      if((addr=OwGetCellAddr(x,y))!=NULL)
         *addr=*c;
      else
         SetScreenCell(sx,sy,c);
   }
}
void  OwGetCell(int x,int y,Cell *c)
{
   if((unsigned)x<Used->Width && (unsigned)y<Used->Height)
   {
      Cell *addr;
      if((addr=OwGetCellAddr(x,y))!=NULL)
         *c=*addr;
      else
         GetScreenCell(sx,sy,c);
   }
   else
      *c=UPAL[0];
}

void  OwPutString(int x,int y,char *s,Cell *a)
{
   Cell  c=*a;
   OwAbsolute(&x,strlen(s),Used->Width);
   OwAbsolute(&y,1,Used->Height);
   while(*s)
   {
      c.ch=(unsigned char)*s++;
      OwSetCell(x++,y,&c);
   }
}
void  OwPutFormattedString(int x,int y,char *s,Cell *a,...)
{
   va_list  va;
   char buffer[256];
   va_start(va,a);
   vsprintf(buffer,s,va);
   va_end(va);
   OwPutString(x,y,buffer,a);
}

void  OwDrawFrame(int x,int y,unsigned w,unsigned h,unsigned char *f,Cell *a)
      /* for example, "…Õª∫∫»Õº" */
{
   int   i;
   Cell  c=*a;

   if(w<2 || h<2)
      return;

   OwAbsolute(&x,w,Used->Width);
   OwAbsolute(&y,h,Used->Height);

   c.flags|=GRAPHIC;

   c.ch=f[0];           /* upper left corner */
   OwSetCell(x,y,&c);

   c.ch=f[1];           /* upper horizontal line */
   for(i=1;i<w-1;i++)
      OwSetCell(x+i,y,&c);

   c.ch=f[2];           /* upper right corner */
   OwSetCell(x+w-1,y,&c);

   c.ch=f[3];           /* left vertical line */
   for(i=1;i<h-1;i++)
      OwSetCell(x,y+i,&c);

   c.ch=f[4];           /* right vertical line */
   for(i=1;i<h-1;i++)
      OwSetCell(x+w-1,y+i,&c);

   c.ch=f[5];           /* lower left corner */
   OwSetCell(x,y+h-1,&c);

   c.ch=f[6];           /* lower horizontal line */
   for(i=1;i<w-1;i++)
      OwSetCell(x+i,y+h-1,&c);

   c.ch=f[7];             /* lower right corner */
   OwSetCell(x+w-1,y+h-1,&c);
}

void  OwDrawShadow(int x,int y,unsigned w,unsigned h,Cell *a)
{
   Cell  c=*a;
   int   i;

   OwAbsolute(&x,w,Used->Width);
   OwAbsolute(&y,h,Used->Height);

   c.flags|=GRAPHIC;

   c.ch=0xDB;
   for(i=1;i<=h;i++)
      OwSetCell(x+w,y+i,&c);

   c.ch=0xDF;
   for(i=1;i<=w;i++)
      OwSetCell(x+i,y+h,&c);

   c.ch=0xDC;
   OwSetCell(x+w,y,&c);
}

void    OwSetCursorPos(int x,int y)
{
   OwAbsolute(&x,1,UW);
   OwAbsolute(&y,1,UH);

   if(x<Used->Width && y<Used->Height && x>=0 && y>=0)
      MoveCursor(x+UX,y+UY);
}
void    OwClearBox(int x,int y,unsigned w,unsigned h,Cell *c)
{
   register int    i;
   register int    j;

   OwAbsolute(&x,w,UW);
   OwAbsolute(&y,h,UH);

   for(i=0;i<h;i++)
      for(j=0;j<w;j++)
         OwSetCell(x+j,y+i,c);
}
void  OwClear(void)
{
   Window   *wnd=Used;
   
   OwClearBox(FULL,UPAL);
   for(OwUse(wnd->SubWindow); Used; OwUse(Used->SubWindow))
      OwClearBox(FULL,UPAL);
   OwUse(wnd);
}

int   OwGetString(int x,int y,int width,
                  char *buffer,int maxlength,Cell *c)
{
   int      length;
   int      pos;
   int      shift=0;
   int      i;
   int      key;
   int      start=1;
   Cell     cell=*c;

   OwAbsolute(&x,width,UW);
   OwAbsolute(&y,1,UH);

   length=strlen(buffer);
   pos=length;

   for(;;)
   {
      if(pos-shift<2)
      {
         shift=pos-2;
      }
      else if(pos-shift>=width-2)
      {
         shift=pos-(width-2)+1;
         if(length-shift<width-1)
            shift=length-(width-1);
      }
      if(shift<0)
         shift=0;

      for(i=0; i<width; i++)
      {
         if(i+shift<length)
            cell.ch=buffer[i+shift];
         else
            cell.ch=' ';
         OwSetCell(x+i,y,&cell);
      }

      OwSetCursorPos(x+pos-shift,y);

      switch(key=ReadKey())
      {
      case(K_HOME):
         pos=0;
         shift=0;
         break;
      case(K_END):
         pos=length;
         break;
      case(K_LEFT):
         if(pos>0)
            pos--;
         break;
      case(K_RIGHT):
         if(pos<length)
            pos++;
         break;
      case(K_BS):
         if(pos==0)
            break;
         pos--;
      case(K_DEL):
         if(pos==length)
            break;
         for(i=pos; i<length; i++)
            buffer[i]=buffer[i+1];
         length--;
         if(shift>0 && length-shift<width)
            shift--;
         break;
      case(K_ENTER):
      case(K_CTRL_M):
      case(K_ESC):
         goto stop;
      case(K_CTRL_P):
         key=ReadKey();
         if(!IsFunctionKey(key))
            goto insert;
         break;
      default:
         if(IsFunctionKey(key) || iscntrl(key))
            break;
insert:  if(start)
         {
            length=pos=shift=0;
            buffer[0]='\0';
         }
         if(length>=maxlength)
            break;
         length++;
         for(i=length; i>pos; i--)
            buffer[i]=buffer[i-1];
         buffer[pos++]=(char)key;
      }
      start=FALSE;
   }
stop:
   return(key);
}
