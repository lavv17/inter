#include	<stdarg.h>
#include        <ctype.h>
#include	<alloc.h>
#include	<string.h>
#include	"ow.h"

int	sprintf(char*,char*,...);

/****************************************************************************/
/*      The objects:                                               	    */
/*              Name     Usage                                              */
/*              -------  -------------------------------------------------- */
/*              frame    Add(&frame,x,y,width,height,type,color);           */
/*              box      Add(&box,x,y,width,height,ch,color);               */
/*              string   Add(&string,x,y,text,color);                       */
/*              shadow   Add(&shadow,x,y,w,h,shade_color);         	    */
/*              button   Add(&button,x,y,hot,text,action,normal,accent);    */
/*		menu	 Add(&menu,menu_ptr,palette);		    	    */
/*		choice	 Add(&choice,x,y,max,intptr, ...);                   */
/*		option	 Add(&option,x,y,intptr,text);		    	    */
/*		getstr	 Add(&getstr,x,y,width,buffer,maxlen,filter,accept, */
/*						    key_on_enter,attrib);   */
/****************************************************************************/

struct  frame_info
{
	char    	*frame;
	sbyte		x,y;
	byte		w,h,c;
};
struct	frame_params
{
	int		x,y;
	unsigned	w,h;
	char		*frame;
	int		c;
};
static int     	create_frame (ptr, param)
struct frame_info       **ptr;
struct frame_params	*param;
{
	New (*ptr);
	Correct (&(param->x), param->w, UW);
	Correct (&(param->y), param->h, UH);
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;
	(*ptr)->w = param->w;
	(*ptr)->h = param->h;
	(*ptr)->frame = param->frame;
	(*ptr)->c = param->c;
	return(OKAY);
}
int    	free_mem (ptr)
void	*ptr;
{
	free (ptr);
	return (OKAY);
}
static int     	display_frame (ptr)
struct frame_info       *ptr;
{
	DrawFrame (ptr->x, ptr->y, ptr->w, ptr->h, ptr->frame, ptr->c);
	return (OKAY);
}
ObjEntry        frame = {create_frame, free_mem, display_frame};

struct  box_info
{
	sbyte		x,y;
	byte		w,h,a;
	char    	c;
};
struct	box_params
{
	int		x,y;
	unsigned	w,h;
	int		c;
	int		a;
};
static int     	create_box(ptr,param)
struct box_info 	**ptr;
struct box_params	*param;
{
	New (*ptr);
	Correct (&(param->x), param->w, UW);
	Correct (&(param->y), param->h, UH);
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;
	(*ptr)->w = param->w;
	(*ptr)->h = param->h;
	(*ptr)->c = param->c;
	(*ptr)->a = param->a;
	return (OKAY);
}
static int     	display_box (ptr)
struct box_info *ptr;
{
	ClearBox (ptr->x, ptr->y, ptr->w, ptr->h, ptr->c, ptr->a);
	return (OKAY);
}
ObjEntry        box = {create_box, free_mem, display_box};

struct  string_info
{
	char    *str;
	sbyte	x,y;
	byte	a;
};
struct	string_params
{
	int	x,y;
	char	*str;
	int	a;
};
static int     	create_string (ptr, param)
struct string_info      **ptr;
struct string_params	*param;
{
	New (*ptr);
	Correct (&(param->x), strlen(param->str), UW);
	Correct (&(param->y), 1, UH);
	(*ptr)->str = param->str;
	(*ptr)->a = param->a;
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;
	return(OKAY);
}
static int	display_string(ptr)
struct string_info      *ptr;
{
	PutString (ptr->x, ptr->y, ptr->str, ptr->a);
	return (OKAY);
}
ObjEntry        string = {create_string, free_mem, display_string};

struct  shadow_info
{
	sbyte	x,y;
	byte	w,h,a;
};
struct	shadow_params
{
	int		x,y;
	unsigned	w,h;
	int		a;
};
static int	create_shadow (ptr, param)
struct shadow_info	**ptr;
struct shadow_params	*param;
{
	New (*ptr);
	Correct (&(param->x), param->w, UW);
	Correct (&(param->y), param->h, UH);
	(*ptr)->w = param->w;
	(*ptr)->h = param->h;
	(*ptr)->a = param->a;
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;
	return (OKAY);
}
static int	display_shadow (ptr)
struct shadow_info    *ptr;
{
	DrawShadow (ptr->x, ptr->y, ptr->w, ptr->h, ptr->a);
	return (OKAY);
}
ObjEntry        shadow = {create_shadow, free_mem, display_shadow};

char	AltChar[]=
	"\0\x1B" "1234567890-=\b"
	"\tQWERTYUIOP[]\r"
	"\0ASDFGHJKL;'`"
	"\0\\ZXCVBNM,./\0*"
	"\0 ";

void	PutItem(x,y,normal,accent,text,len,offset)
char	*text;
{
	while(offset-- >0)
		SetCell(x++,y,' ',normal),len--;
	while(*text && --len)
	{
		if(*text=='&' && text[1] && *(++text)!='&')
			SetCell(x++,y,*(text++),accent);
		else
			SetCell(x++,y,*(text++),normal);
	}
	while(len-- >0)
		SetCell(x++,y,' ',normal);
}
int	TestKey(text,key)
char	*text;
{
	if((key&0xff)==0)
	{
		key>>=8;
		if(key>=sizeof(AltChar))
			return(FALSE);
		key=AltChar[key];
	}
	key&=0xff;
	while(*text)
	{
		if(*text=='&' && *(++text)!='&')
			return(toupper(*text)==toupper(key));
		text++;
	}
	return(FALSE);
}
int	ItemLen(text)
char	*text;
{
	int	len=0;

	while(*text)
	{
		if(*text++=='&' && *text)
			text++;
		len++;
	}
	return(len);
}

struct  button_info
{
	char    	*str;
	sbyte		x,y;
	int		(*action)(void);
};
struct	button_params
{
	int	x,y;
	char	*str;
	int	(*action)(void);
};
static	int     create_button (ptr,param)
struct button_info	**ptr;
struct button_params	*param;
{
	New (*ptr);

	Correct (&(param->x), ItemLen(param->str), UW);
	Correct (&(param->y), 1, UH);

	(*ptr)->str = param->str;
	(*ptr)->action = param->action;
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;

	return (OKAY);
}
static	int     display_button (ptr)
struct  button_info      *ptr;
{
	PutItem (ptr->x, ptr->y, BUTTON, BBUTTON, ptr->str, 0, 0);
	DrawShadow (ptr->x, ptr->y, ItemLen(ptr->str), 1, SHADOW);
	return (OKAY);
}
static	int	activate_button (ptr, key)
struct	button_info	*ptr;
int			key;
{
	int	display_button(void*);

	if(key!=-1 && key!=K_TAB && !TestKey(ptr->str,key))
		return(-1);

	CursorOFF();

	if(key==K_TAB || key==-1)
	{
	    SetCell (ptr->x, ptr->y, '\020', BBUTTON);
	    SetCell (ptr->x+ItemLen(ptr->str)-1, ptr->y, '\021', BBUTTON);
	}
	if (TestKey(ptr->str,key) || (key=GetKey(0))==K_ENTER)
	{
	    ClearBox (ptr->x, ptr->y, ItemLen(ptr->str)+1, 2, ' ', NORMAL);
	    PutItem (ptr->x+1, ptr->y, BUTTON, BBUTTON, ptr->str, 0, 0);
	    key=ptr->action();
	}
	display_button(ptr);
	return(key);
}
static int	getlimits_button(ptr,info)
struct button_info	*ptr;
Limits			*info;
{
	info->Left = ptr->x;
	info->Top = ptr->y;
	info->Right = ptr->x+ItemLen(ptr->str)-1;
	info->Bottom = ptr->y;
	return (OKAY);
}
ObjEntry button = {create_button, free_mem, display_button, NULL, activate_button,
		NULL, getlimits_button};

int	ok(void)
{
	return(K_ENTER);
}
int	can(void)
{
	return(K_ESC);
}

/****************************************************************************/
struct	choice_info
{
	sbyte		x,y;
	byte		n;
	int		*sw;
	int		newsw;
};
struct	choice_params
{
	int	x,y;
	int	*sw;
	/* ... */	/* text strings */
};
static int	create_choice(ptr,param)
struct choice_info	**ptr;
struct choice_params	*param;
{
	int	n;

	for (n=0; ((char**)(param+1))[n]; n++);	/* count strings */

	if(n==0)
		return(CANCEL);			/* nothing to do */

	*ptr = smalloc (sizeof(struct choice_info) + n*sizeof(char*));

	Correct (&(param->x), 3, UW);
	Correct (&(param->y), n, UH);

	(*ptr)->sw = param->sw;
	(*ptr)->n = n;
	(*ptr)->x = param->x;
	(*ptr)->y = param->y;

	while(n--)
		((char**)(*ptr+1)) [n] = ((char**)(param+1)) [n];

	return (OKAY);
}
static int	display_choice(ptr)
struct choice_info	*ptr;
{
	int	y;

	ptr->newsw = *(ptr->sw);
	for (y=0; y<ptr->n; y++)
	{
		PutString(ptr->x+4,y+ptr->y,((char**)(ptr+1))[y],NORMAL);
		PutString(ptr->x,y+ptr->y,"( )",NORMAL);
		if(ptr->newsw==y)
			SetCell(ptr->x+1,y+ptr->y,'\007',NORMAL);
	}
	return(OKAY);
}
static	int	activate_choice(ptr,key)
struct	choice_info	*ptr;
int			key;
{
	int	y;

	if(key!=-1 && key!=K_TAB)
		return(-1);

	y=ptr->newsw;

	while(1)
	{
		SetCursorPos(ptr->x+1,ptr->y+y);
		switch(key=GetKey(0))
		{
			case(K_UP):
				if(--y<0)
					goto ret;
				break;
			case(K_DOWN):
				if(++y>=ptr->n)
					goto ret;
				break;
			case(K_SPACE):
				SetCell(ptr->x+1,y+ptr->y,'\007',NORMAL);
				SetCell(ptr->x+1,ptr->y+ptr->newsw,' ',NORMAL);
				ptr->newsw=y;
				break;
			default:
				goto ret;
		}
	}
ret:
	CursorOFF();
	return(key);
}
static	int	getlimits_choice(ptr,info)
struct	choice_info	*ptr;
Limits			*info;
{
	info->Left=ptr->x;
	info->Top=ptr->y;
	info->Right=ptr->x+2;
	info->Bottom=ptr->y+ptr->n-1;
	return(OKAY);
}
static int	memchanged_choice(ptr,mem)
struct choice_info	*ptr;
int			*mem;
{
	if(mem==ptr->sw)
	{
		int	y;

		ptr->newsw=*(ptr->sw);
		for(y=0; y<ptr->n; y++)
			SetCell(ptr->x+1,y+ptr->y,
					y==ptr->newsw?'\007':' ',NORMAL);
	}
	return(OKAY);
}
int	chosen_choice(ptr,sig)
struct choice_info	*ptr;
int			sig;
{
	if(sig==OKAY)
	{
		*(ptr->sw)=ptr->newsw;
		Signal(ptr->sw);
	}
	else if(sig==CANCEL)
	{
		int	y;
		ptr->newsw=*(ptr->sw);
		for(y=0; y<ptr->n; y++)
		    SetCell(ptr->x+1,y+ptr->y,y==ptr->newsw?'\007':' ',NORMAL);
	}
	return(OKAY);
}
ObjEntry        choice = {create_choice, free_mem, display_choice, NULL,
	      activate_choice, chosen_choice, getlimits_choice, memchanged_choice};
/****************************************************************************/
struct	option_info
{
	sbyte	x,y;
	int		*sw;
	int		newsw;
	char		*text;
};
struct	option_params
{
	int	x,y;
	int	*sw;
	char	*text;
};
static	int     create_option(ptr,param)
struct	option_info	**ptr;
struct	option_params	*param;
{
	New(*ptr);

	Correct(&(param->x),3,UW);
	Correct(&(param->y),1,UH);

	(*ptr)->x=param->x;
	(*ptr)->y=param->y;
	(*ptr)->sw=param->sw;
	(*ptr)->text=param->text;

	return(OKAY);
}
static	int     display_option(ptr)
struct  option_info      *ptr;
{
	ptr->newsw=*(ptr->sw);
	PutString(ptr->x+4,ptr->y,ptr->text,NORMAL);
	PutString(ptr->x,ptr->y,"[ ]",NORMAL);
	if(ptr->newsw)
		SetCell(ptr->x+1,ptr->y,'x',NORMAL);
	return(OKAY);
}
static	int	activate_option(ptr,key)
struct	option_info	*ptr;
int			key;
{
	if(key!=-1 && key!=K_TAB)
		return(-1);
	SetCursorPos(ptr->x+1,ptr->y);
	while ((key=GetKey(0))==K_SPACE)
	{
		ptr->newsw=!ptr->newsw;
		SetCell (ptr->x+1,ptr->y,ptr->newsw?'x':' ',NORMAL);
	}
	CursorOFF();
	return (key);
}
int	getlimits_option(ptr,info)
struct	option_info	*ptr;
Limits			*info;
{
	info->Left=ptr->x;
	info->Top=ptr->y;
	info->Right=ptr->x+2;
	info->Bottom=ptr->y;
	return(OKAY);
}
int	memchanged_option(ptr,mem)
struct option_info	*ptr;
int			*mem;
{
	if(mem==ptr->sw)
	{
		ptr->newsw=*(ptr->sw);
		SetCell(ptr->x+1,ptr->y,ptr->newsw?'x':' ',NORMAL);
	}
	return(OKAY);
}
int	chosen_option(ptr,sig)
struct option_info	*ptr;
int			sig;
{
	if(sig==OKAY)
	{
		*(ptr->sw)=ptr->newsw;
		Signal(ptr->sw);
	}
	else if(sig==CANCEL)
	{
		ptr->newsw=*(ptr->sw);
		SetCell(ptr->x+1,ptr->y,ptr->newsw?'x':' ',NORMAL);
	}
	return(OKAY);
}
ObjEntry        option = {create_option, free_mem, display_option, NULL,
	  activate_option, chosen_option, getlimits_option, memchanged_option};
/****************************************************************************/
struct	getstr_info
{
	sbyte	x,y;
	byte	w;
	char	*buffer;
	char	*new;
	int	(*filter)(char,char*,int);
	int	(*accept)(char*);
	unsigned maxlen;
	int	on_enter;
	int	pos;
};
struct	getstr_params
{
	int	x,y;
	unsigned w;
	char	*buffer;
	unsigned maxlen;
	int	(*filter)(char,char*,int);
	int	(*accept)(char*);
	int	on_enter;
};
static int	create_getstr(ptr,param)
struct getstr_info	**ptr;
struct getstr_params	*param;
{
	New(*ptr);

	Correct(&(param->x),param->w,UW);
	Correct(&(param->y),1,UH);

	(*ptr)->x=param->x;
	(*ptr)->y=param->y;
	(*ptr)->w=param->w;
	(*ptr)->buffer=param->buffer;
	(*ptr)->filter=param->filter;
	(*ptr)->accept=param->accept;
	(*ptr)->maxlen=param->maxlen;
	(*ptr)->on_enter=param->on_enter;
	(*ptr)->new=smalloc(param->maxlen+1);

	strcpy((*ptr)->new,(*ptr)->buffer);

	return(OKAY);
}
static int	destroy_getstr(ptr)
struct getstr_info	*ptr;
{
	free(ptr->new);
	free(ptr);
	return(OKAY);
}
static int	display_getstr(ptr)
struct getstr_info	*ptr;
{
	register int	i;
	int		len=strlen(ptr->new);

	for(i=0; i<ptr->w; i++)
	{
		if(i>=len)
			SetCell(ptr->x+i,ptr->y,' ',INPUT);
		else
			SetCell(ptr->x+i,ptr->y,ptr->new[i],INPUT);
	}
	SetCell(ptr->x-1,ptr->y,' ',NORMAL);
	SetCell(ptr->x+ptr->w,ptr->y,' ',NORMAL);

	return(OKAY);
}
static struct getstr_info	*ci;
static int	getstr_ikey(int key,char*buffer,int pos)
{
	(void)buffer;
	ci->pos=pos;
	return(key);
}
static int	activate_getstr(ptr,key)
struct getstr_info	*ptr;
int			key;
{
	int	display_getstr(void*);

	if(key!=-1 && key!=K_TAB)
		return(-1);

	ci=ptr;

	key=GetString(ptr->x,ptr->y,ptr->w,ptr->new,ptr->maxlen,ptr->filter,
		getstr_ikey,INPUT,key==-1?ptr->pos:-1);

	display_getstr(ptr);

	if(key==K_ENTER)
		return(ptr->on_enter);
	return(key);
}
static int	getlimits_getstr(ptr,info)
struct getstr_info	*ptr;
Limits			*info;
{
	info->Left = ptr->x;
	info->Top = ptr->y;
	info->Right = ptr->x+ptr->w-1;
	info->Bottom = ptr->y;
	return (OKAY);
}
static int	chosen_getstr(ptr,key)
struct getstr_info	*ptr;
{
	if(key==OKAY)
	{
		if(ptr->accept && !ptr->accept(ptr->new))
			return(ERROR);	/* continue dialogue */
		if(strcmp(ptr->buffer,ptr->new))
		{
			strcpy(ptr->buffer,ptr->new);
			Signal(ptr->buffer);
		}
	}
	else
	{
		strcpy(ptr->new,ptr->buffer);
		display_getstr(ptr);
	}
	return(OKAY);
}
static int	memchanged_getstr(ptr,mem)
struct getstr_info	*ptr;
char			*mem;
{
	if(mem==ptr->buffer)
	{
		strcpy(ptr->new,ptr->buffer);
		display_getstr(ptr);
	}
	return(OKAY);
}
ObjEntry        getstr = {create_getstr, destroy_getstr, display_getstr, NULL,
	  activate_getstr, chosen_getstr, getlimits_getstr, memchanged_getstr};
/****************************************************************************/
struct	varstr_info
{
	int	x,y;
	byte	a;
	char	*buffer;
	char	*format;
	unsigned oldlen;
};
struct	varstr_params
{
	int	x,y;
	char	*format;
	char	*buffer;
	int	a;
};
static int	create_varstr(ptr,param)
struct varstr_info	**ptr;
struct varstr_params	*param;
{
	New(*ptr);

	(*ptr)->x=param->x;
	(*ptr)->y=param->y;
	(*ptr)->a=param->a;
	(*ptr)->buffer=param->buffer;
	(*ptr)->format=param->format;
	(*ptr)->oldlen=0;

	return(OKAY);
}
static int	destroy_varstr(ptr)
struct varstr_info	*ptr;
{
	free(ptr);
	return(OKAY);
}
static int	display_varstr(ptr)
struct varstr_info	*ptr;
{
	char	buffer[256];
	int	len;

	sprintf(buffer,ptr->format,ptr->buffer);
	len=strlen(buffer);
	if(len<ptr->oldlen)
		ClearBox(ptr->x,ptr->y,ptr->oldlen,1,' ',ptr->a);
	PutString(ptr->x,ptr->y,buffer,ptr->a);
	ptr->oldlen=len;
	return(OKAY);
}
static int	memchanged_varstr(ptr,mem)
struct varstr_info	*ptr;
char			*mem;
{
	int	display_varstr(void*);

	if(mem==ptr->buffer)
		display_varstr(ptr);
	return(OKAY);
}
ObjEntry        varstr = {create_varstr, destroy_varstr, display_varstr, NULL,
				  NULL, NULL, NULL, memchanged_varstr};

struct	varint_info
{
	int	x,y;
	byte	a;
	int	*number;
	char	*format;
	int	oldlen;
};
struct	varint_params
{
	int	x,y;
	char	*format;
	int	*number;
	int	a;
};
static int	create_varint(ptr,param)
struct varint_info	**ptr;
struct varint_params	*param;
{
	New(*ptr);

	(*ptr)->x=param->x;
	(*ptr)->y=param->y;
	(*ptr)->a=param->a;
	(*ptr)->number=param->number;
	(*ptr)->format=param->format;
	(*ptr)->oldlen=0;

	return(OKAY);
}
static int	destroy_varint(ptr)
struct varint_info	*ptr;
{
	free(ptr);
	return(OKAY);
}
static int	display_varint(ptr)
struct varint_info	*ptr;
{
	char	buffer[256];
	int	len;

	sprintf(buffer,ptr->format,*(ptr->number));
	len=strlen(buffer);
	if(len<ptr->oldlen)
		ClearBox(ptr->x,ptr->y,ptr->oldlen,1,' ',ptr->a);
	PutString(ptr->x,ptr->y,buffer,ptr->a);
	ptr->oldlen=len;
	return(OKAY);
}
static int	memchanged_varint(ptr,mem)
struct varint_info	*ptr;
int			*mem;
{
	int	display_varint(void*);

	if(mem==ptr->number)
		display_varint(ptr);
	return(OKAY);
}
ObjEntry        varint = {create_varint, destroy_varint, display_varint, NULL,
				  NULL, NULL, NULL, memchanged_varint};
/*****************************************************************************/
struct	progress_info
{
	int	x,y;
	byte	width;
	byte	a;
	int	*percents;
};
struct	progress_params
{
	int	x,y;
	int	width;
	int	*percents;
	int	a;
};
static int	create_progress(ptr,param)
struct progress_info	**ptr;
struct progress_params	*param;
{
	New(*ptr);

	(*ptr)->x=param->x;
	(*ptr)->y=param->y;
	(*ptr)->width=param->width;
	(*ptr)->percents=param->percents;
	(*ptr)->a=param->a;

	return(OKAY);
}
static int	display_progress(ptr)
struct progress_info	*ptr;
{
	int	i;
	for(i=0; i<ptr->width; i++)
		SetCell(ptr->x+i,ptr->y,(*(ptr->percents) <=
			(i*100+ptr->width/2)/(ptr->width))?'°':'Û',ptr->a);
	return(OKAY);
}
static int	memchanged_progress(ptr,mem)
struct progress_info	*ptr;
int			*mem;
{
	int	display_progress(void*);

	if(mem==ptr->percents)
		display_progress(ptr);
	return(OKAY);
}
ObjEntry        progress = {create_progress, free_mem, display_progress, NULL,
				  NULL, NULL, NULL, memchanged_progress};
