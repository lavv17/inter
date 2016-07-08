#if !defined(MSDOS)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>

long  hz;
unsigned long  Timer(void)
{
   struct tms  t;
   clock_t     clock;
   
   clock=times(&t);
   return((unsigned long)(clock*(1000.0/hz)));
}

void  TimerInit(void)
{
   hz=sysconf(_SC_CLK_TCK);
   if(hz==0)
   {
      fprintf(stderr,"cannot get system clock frequency\n");
      exit(1);
   }
}

#else
#include <dos.h>

unsigned long  Timer(void)
{
/*   const unsigned long timer_freq=1193182UL;*/
   unsigned long  time;
   union REGS  r;
   
   r.h.ah=0;
   int86(0x1A,&r,&r);
   time=((unsigned)r.w.dx+(((unsigned long)(unsigned)r.w.cx)<<16))*55UL;
   
/*   disable();
   outportb(0x43,0);
   time=(unsigned long)(unsigned)inportb(0x40);
   time+=(unsigned long)(unsigned)inportb(0x40)<<8;
   time=(time*1000UL)/timer_freq;
   time=((unsigned long)(unsigned)peek(0,0x46C)
         +(((unsigned long)(unsigned)peek(0,0x46E))<<16))*55UL;
   enable();*/
   return(time);
}

void  TimerInit(void) {}

#endif
