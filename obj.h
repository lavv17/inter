/*
**	File OBJ.H
**	Definitions for objects of window interface
**	Copyright (c) Alexander V. Lukyanov, 1993
*/

#ifndef	_OBJ_INCLUDED
#define	_OBJ_INCLUDED

extern	ObjEntry	string,frame,box,shadow,button,menu,callmenu,swtch,option;
extern	ObjEntry	getstr,varstr,varint,progress;

typedef struct	sr_menu
{
	char	*text;	/* the text of the item, NULL means end of (sub)menu */
	byte	flag;	/* either SUBM,FUNC,WIND,FUNC+HIDE,or WIND+HIDE	*/
	union	deel
	{
		void	(*func)(void);	/* the function to call		*/
		Window	*wind;		/* the window to activate	*/
	} action;
	int	key;	/* the global key, -1 means no key	*/
	int	*sw;	/* pointer to switch, NULL implies always ON	*/
} Menu;

void	PutItem(int x,int y,int normal,int accent,char*text,int len,int offset);
int	TestKey(char*text,int key);
int	ItemLen(char*text);

int	ok(void); 	/* returns K_ENTER */
int	can(void);	/* returns K_ESC */

#define	SUBM	1	/* pull down a submenu 	*/
#define	WIND	2	/* activate a window	*/
#define	FUNC	4	/* call a function	*/
#define	HIDE	8	/* it is used with the FUNC or the WIND to hide menu
			    before doing the action	*/
#define	NOKEY	-1

#define	ON	1
#define	OFF	0

#define	AddKey(x,y,hot,text,action,norm,acc)	Add(&key,x,y,hot,text,action,norm,acc)
#define	AddMenu(mnu,k,pal)			Add(&menu,mnu,k,pal)
#define	AddOption(x,y,sw,text_x,txt)		Add(&option,x,y,sw,text_x,txt)

#endif
