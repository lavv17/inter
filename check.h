/*____________________________________________________________________________
**
** File:       check.h
**____________________________________________________________________________
*/

extern   void  *CheckPtr(void*ptr);
extern   void  AddMemoryErrorHandler(void (*Handler)(void),int Preceedence);
