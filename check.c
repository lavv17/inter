/*____________________________________________________________________________
**
** File:       check.c
**____________________________________________________________________________
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "check.h"

#define  MAX_ERROR_HANDLERS   16

struct
{
   void  (*Handler)(void);
   int   Preceedence;
}  MemoryErrorHandler[MAX_ERROR_HANDLERS];
int   MemoryErrorHandlersNum=0;

void  *CheckPtr(void *ptr)
{
   if(ptr==NULL)
   {
      int   i;
      for(i=0; i<MemoryErrorHandlersNum; i++)
         MemoryErrorHandler[i].Handler();
      fprintf(stderr,"\n\rMemory allocation error - malloc returned NULL\n\r");
      abort();
   }
   return(ptr);
}

void  AddMemoryErrorHandler(void (*Handler)(void),int Preceedence)
{
   int   i;
   for(i=0; i<MemoryErrorHandlersNum; i++)
      if(MemoryErrorHandler[i].Handler==Handler)
      {
         MemoryErrorHandlersNum--;
         memmove(MemoryErrorHandler+i,MemoryErrorHandler+i+1,
                 (MemoryErrorHandlersNum-i)*sizeof(*MemoryErrorHandler));
         break;
      }
   assert(MemoryErrorHandlersNum<MAX_ERROR_HANDLERS);
   for(i=0; i<MemoryErrorHandlersNum
            && MemoryErrorHandler[i].Preceedence<=Preceedence; i++);
   memmove(MemoryErrorHandler+i+1,MemoryErrorHandler+i,
           (MemoryErrorHandlersNum-i)*sizeof(*MemoryErrorHandler));
   MemoryErrorHandler[i].Handler=Handler;
   MemoryErrorHandler[i].Preceedence=Preceedence;
   MemoryErrorHandlersNum++;
}
