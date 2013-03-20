#include <stdio.h>
#include "inter.h"
#include "ow.h"
#include "keys.h"
#include "colors.h"

main()
{
   Rgb   brgb={0,0xFF,0};
   Cell  back[]={{0,'°'}};
   Cell  pal[]={{COLOR,' ',{0xFF,0,0},{0,0,0}}};
   Cell  pal_a[]={{COLOR,' ',{0xFF,0,0},COLOR_DOSGREEN}};
   Window   *w,*a;

   int   i;

   OpenInterface();
   OwInitialize(back);
   OwDisplay(Root);
   OwCreate(&w,MIDDLE,MIDDLE,20,10,pal);
   OwCreateArea(&a,MIDDLE,MIDDLE,10,5,pal_a,w);
   OwDisplay(w);
   ReadKey();

   OwUse(w);
   for(i=0; i<256; i+=4)
   {
      pal[0].background.r=brgb.r*i/256;
      pal[0].background.g=brgb.g*i/256;
      pal[0].background.b=brgb.b*i/256;
      OwClear();
      OwPutString(2,2,"Hello !",&pal[0]);
      Sync();
   }

   WaitKey(3000);
   
   OwMove(MIDDLE-3,MIDDLE-3);
   
   WaitKey(1000);
   
   OwMove(MIDDLE+3,MIDDLE+3);
   
   WaitKey(500);

   OwCopyArea(FULL,1,1,TRUE);

   ReadKey();
   
   OwUse(a);
   OwClose();
   OwClose();

   OwExit();
   CloseInterface();
   return 0;
}
