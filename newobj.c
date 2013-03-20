#include	<stdlib.h>
#include	<alloc.h>
#include	<io.h>
#include	<fcntl.h>
#include	"ow.h"
#include	<string.h>

# define	SELECTED	1
/*
typedef	struct dmenu
{
	void		*info;
	struct dmenu	*next;
}
	DMenu;

void	DinamicMenu(x,y,menu,width,height,length,GetText,GetFlag,ikey,norm,sel)
int		x,y;
DMenu		*menu;
unsigned	width,height,length;
char		*(*GetText)(void *);
int		(*GetFlag)(void *);
int		(*ikey)(void *);
int		norm,sel;
{
}
*/
/*****************************************************************************/
extern	char	*sys_errlist[];
extern	int	errno;

char	infile[65]="";
char	outfile[65]="";

byte    p[]={0x70,0x7F,0x07,0x0F,0x07,0x0f,0x70,0x70,0x0F};

int	accept_file(char*name)
{
	return(*name?TRUE:FALSE);
}

void	hlp(void)
{
	Window	*wnd;

	Create(&wnd,MIDDLE,MIDDLE,30,7,p);
	Add(&string,MIDDLE,2,"Sorry, help is unavailable",NORMAL);
	Add(&frame,FULL,"…Õª∫∫»Õº",FRAME);
	Add(&button,MIDDLE,4,"   Ok   ",ok,REVERSE,BREVERSE);
	Display(wnd);
	Activate();
	Destroy(wnd);
}

void	Error (char *file)
{
	Window	*wnd;
	char	*msg=(errno>0)?sys_errlist[errno]:"There is no room to copy";
	char	buf[100];
	int	len=strlen(msg);

	if(msg[len]=='\n')
		msg[len]='\0';

	sprintf(buf,"%s: %s",file,msg);

	Create(&wnd,MIDDLE,MIDDLE+5,strlen(buf)+4,7,p);
	Add(&frame,FULL,"…Õª∫∫»Õº",FRAME);
	Add(&string,MIDDLE,0," Error ",NORMAL);
	Add(&string,MIDDLE,2,buf,NORMAL);
	Add(&button,MIDDLE,4,"   &Ok   ",ok,REVERSE,BREVERSE);
	Display(wnd);
	Activate();
	_SetCursorPos(0,Screen->Height-1);
	DestroyAll();
	CursorON();
	exit(1);
}
void	copy()
{
	Window	*wnd;
	int	progr=0;
	int	fd1,fd2;
	long	len;
	int	cnt1,cnt2;
	char	buffer[0x100];

	Create(&wnd,MIDDLE,MIDDLE,max(strlen(infile)+27,strlen(outfile))+4,7,p);
	Add(&frame,FULL,"…Õª∫∫»Õº",FRAME);
	Add(&string,MIDDLE,0," Copy ",NORMAL);
	Add(&varstr,MIDDLE,1,"Copying the file `%s' to",infile,NORMAL);
	Add(&varstr,MIDDLE,2,"the file `%s'",outfile,NORMAL);
	Add(&varint,RIGHT-2,UH-2,"%3d%%",&progr,NORMAL);
	Add(&progress,2,UH-3,UW-4,&progr,NORMAL);
	Display(wnd);

	fd1=_open(infile,O_RDONLY);
	if(fd1==-1)
		Error(infile);
	fd2=_creat(outfile,0);
	len=filelength(fd1);
	if(len==0)
	{
		progr=100;
		Signal(&progr);
		_close(fd1);
		_close(fd2);
		return;
	}
	do
	{
		cnt1=_read(fd1,buffer,sizeof(buffer));
		if(cnt1==-1)
			Error(infile);
		if(cnt1==0)
			break;
		cnt2=_write(fd2,buffer,cnt1);
		if(cnt2<cnt1)
			Error(outfile);

		progr=(int)((tell(fd1)*100+len/2)/len);
		Signal(&progr);
	}
	while(TRUE);
	Destroy(wnd);
}
void	nop(void){}

int	zero=0;
Menu	mymenu[]=
{
	{ " &File ",SUBM },
		{ "&Load      F3",FUNC,nop,K_F3 },
		{ "---" },
		{ "&Pick  Alt-F3",FUNC,nop,K_ALT_F3 },
		{ NULL },
	{ " &Edit ",FUNC+HIDE,nop },
	{ " &Run ",SUBM },
		{ "&Run            Ctrl-F9",FUNC+HIDE,nop,K_CTRL_F9,&zero },
		{ "&Program reset  Ctrl-F2",FUNC+HIDE,nop,K_CTRL_F2,&zero },
		{ "&Trace into          F7",FUNC+HIDE,nop,K_F7 },
		{ NULL },
	{ NULL }
};

int	main()
{
	Window	*wnd;

	DefineScreen(p);
	Add(&menu,mymenu,p);
	Add(&box,0,1,UW,UH-2,'±',REVERSE);
	Display(Screen);

	Create(&wnd,MIDDLE,MIDDLE,50,10,p);
	Add(&callmenu);
	Add(&frame,FULL,DOUBLE,FRAME);
	Add(&string,MIDDLE,0," Copy ",NORMAL);
	Add(&string,2,1,"Source file",NORMAL);
	Add(&getstr,3,2,UW-6,infile,64,NULL,accept_file,K_TAB,INPUT);
	Add(&string,2,4,"Destination file",NORMAL);
	Add(&getstr,3,5,UW-6,outfile,64,NULL,accept_file,K_ENTER,INPUT);
	Add(&button,MIDDLE-10,UH-3,"  C&opy  ",ok,REVERSE,BREVERSE);
	Add(&button,MIDDLE,   UH-3," &Cancel ",can,REVERSE,BREVERSE);
	Add(&button,MIDDLE+10,UH-3,"  &Help  ",hlp,REVERSE,BREVERSE);
	Display(wnd);
	if(Activate()==OKAY)
	{
		Destroy(wnd);
		copy();
	}
	DestroyAll();
	_SetCursorPos(0,Screen->Height-1);
	CursorON();
	return 0;
}


