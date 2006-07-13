/*	Program		: %M% (%Q%)
 *      Version		: V %R%.%L%
 *      Date last vers.	: %E%,	%U%
 *      Author		: H.Merijn Brand
 *	Description	: Menu Manager
 *
 * Copyright (C) 1986-2006 H.Merijn Brand, PROCURA B.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *      Edit	Programmer	Date	  Reason
 *      ------- --------------- --------- -----------------------------------
 *	V 1.0	M. Feldbrugge   21 Nov 86 Initial version
 *	V 2.0	H.M. Brand	11 Sep 91 Added dialogues; recursive calls;
 *					  environments; dialogue options and
 *					  -default prefixes; format changes;
 *					  presentation and U/I; inhibit loops;
 *					  on-line help; F-keys like UNIFY;
 *					  cs7/cs8 support;
 *	V 2.1	H.M. Brand	21 Dec 93 Aliases, Background, Pipes,
 *					  Quickshell, SCCS, Required/Comment
 *
 *      Run procedure :
 *              V 1.0 : cshmen [group_name]
 *              V 2.0 : cshmen [-e] [-d] [-v] [-n] [-7] [base-menu]
 *		V 2.1 : zie help ()
 *
 *      Special hardware-/software needs of program :
 *		If base-menu is omitted in the command-line, $HOME must
 *		be set in the environment and either $MENU_DIR/.cshmen OR
 *              $HOME/.cshmen must exist.
 *		Menu format:
 *		<Menu>	       :: <Menu-Line> | <Menu> \n <Menu-Line>
 *		<Menu-Line>    :: <Ident>:<Label>:<Command>:<Type>:<Help-File>
 *		<Ident>	       :: <Header-Ident> | <Action-Ident> | <Exit-Ident>
 *		<Header-Ident> :: H
 *		<Action-Ident> :: <Ident-char> | <Ident-char>=<Alias-char>
 *		<Ident-char>   :: 0 .. 9 | A .. F
 *		<Alias-char>   :: ! .. 9 | ; .. ~
 *		<Exit-Ident>   :: S | Q
 *		<Label>	       :: Text, no colons max 46 char
 *		<Command>      :: M <Menu-Name> | <Cmds> | <Cmds> ; <Cmds>
 *		<Cmds>         :: <Action> [<Where>] [<Redirect>]
 *		<Action>       :: <Cmd> | <Cmd> \| <Cmd>
 *		<Cmd>          :: <Unix-Cmd> | <Cmd> <Arg>
 *		<Unix-Cmd>     :: See Unix manual section 1
 *		<Where>        :: @directory
 *		<Redirect>     :: <Redir> | <Redirect> <Redir>
 *		<Redir>        :: >file | 2>file | >>file | 2>>file | 2>&1
 *		<Arg>	       :: <Fixed-Arg> | <Quest-Arg>
 *		<Fixed-Arg>    :: See Unix manual for <Unix-Cmd>
 *		<Quest-Arg>    :: ?<Qline>[,c]<Qprompt>[,<Qdef>[,<Qprf>[,<Qopts>[,<Help-File>]]]]]
 *				  Args may contain $ENV like vars
 *		<Qline>        :: 0 .. 9
 *		<Qprompt>      :: Text, no comma's Max 24 char _ is blank
 *		<Qdef>         :: Default answer to Quest
 *		<Qargs>        :: Prefix if answer filled
 *		<Qopts>        :: <Qopt> | <Qopt><Qopt>
 *		<Qopt>         :: S | R | C
 *		<Type>	       :: <inactive> | <Active> | <Active-Wait> | <Disp>
 *		<Inactive>     :: 0
 *		<Active>       :: 1
 *		<Active-Wait>  :: 2
 *		<Disp>         :: 4
 *		<Help-File>    :: File contaning help-text
 *	Example:
======
H:TESTMENU        :env=val env=val:1
3:Args with both  :echo ?0Prompt_default_with_opt,`pwd`,$PRO:2
4:Args            :echo ?0Prompt,vv$ww,$xx,RS ?1Sec_arg,`pwd`,-p | -
			rev @/tmp >yy 2>>zz ; echo ee &:0
6=M:Normal menu   :M M_probevDBA:1:hlp/M_probevDBA
Q:Stop::1
======
 *
 *      Purpose :
 *
 *      Give the end user a menu (with program cq. shell options) to choose from
 *
 *      (m)'86  <211186>  for OSL Almere
 *      (m)'91  <110991>  for Procura Heerhugowaard
 *
 *      no UNIX version is comming near.
 *
 *****************************************************************************/

char VERSION[]   = "cshmen: 3.50 - (m)'06 [13 Jul 2006]\n";
char _SCCSid[]   = "%W%	[%E%]	(m)'06";
char _Version_[] = "#\300\0\0??p\377Nu\0\0(C) Copyright PROCURA B.V. Version V.3.50\n\
System V (m)'06 <13 Jul 2006>";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <curses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define min(a,b)        (((a)<(b))?(a):(b))
#define max(a,b)        (((a)>(b))?(a):(b))
#define isodigit(c)	(isdigit (c) && (c) < '8')

#define NIL             ((char *)0)
#define LCMDMAX         2048
#define LSTART          15
#define LWIDTH          50
#define HTEXT		LWIDTH
#define HWIDTH          (HTEXT + 4)
#define MLINES          16
#define LINEOFF		3
#define HLINE           0
#define QLINE           (MLINES + 1)
#define LLINE           (QLINE + 1)
     /* _A_STANDOUT     (A_REVERSE | A_DIM) */
#define _A_STANDOUT     (A_STANDOUT) /* Select a local standout mode */
#define ACTV_ACT	0x0001
#define ACTV_WAIT	0x0002
#define ACTV_DISP	0x0004

typedef	unsigned char	byte;

extern	void	perror(), *calloc();
extern	int	chdir(), getopt(), optind;
extern	char	*optarg, *getenv(), *getlogin();
#ifndef	RISC
extern	char	*strcat(), *strcpy();
#endif
extern	FILE	*fopen();
extern	WINDOW	*initscr();

static	char	valid_ids[24] = "H0123456789ABCDEFQ";
static	int	lang = 0;	/* 0 = Dutch, 1 = English		*/
static	int	Vflg = 0;	/* TERM = vt320 or equiv.		*/
static	int	Wflg = 1;	/* TERM = wy60  or equiv.		*/
static	int	Xflg = 0;	/* TERM = xterm or equiv.		*/
static	int	Xgui = 0;	/* Use xamen instead			*/
static	int	dflg = 0;	/* debug				*/
static	int	vflg = 0;	/* verbose				*/
static	int	aflg = 0;	/* administrator			*/
static	int	tflg = 0;	/* tijd in beeld			*/
static	int	nflg = 1;	/* no_loops flag			*/
static	int	uflg = 0;	/* show user name in menu		*/
static	int	Key_Home = 0;	/* HOME-key pressed			*/
static	int	keypad_mode = TRUE;
static	int	dialogue_idx;
static	char	answers[16][80];
static	char	*args[256];	/* parsed arguments to command		*/
static	char	*aexp[512];	/* expanded arguments			*/
static	int	expidx = 0;	/* index in above list			*/
typedef	struct {
    char	id;		/* 0-9ab cmd line, h header, q quit	*/
    char	label[LWIDTH + 1];	/* menu label			*/
    char	cmd[LCMDMAX];	/* command line				*/
    int		alias;		/* alias				*/
    int		active;		/* 0 inaccessible, 1 accessible		*/
    char	helpf[128];	/* Help file				*/
    } Menuline;
typedef	struct {
    Menuline	*line[LLINE];
    int		aliascnt;
    char	fname[128];
    int		idx;
    char	*env[10][2];
    } Menu;
static	char	isident[] = { /* + - 0..9 A..Z _ a..z */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
static	int		curm = 0;
static	char		scurtim[16];	/* Current time */
static	Menu		*menus[50];
static	Menuline	ml_shell;	/* Command field for quick-access,
					 * help field for quests */

static	WINDOW	*dbox, *fkeyw, *wworld;
static	struct IND {
    char	*str;
    int		att;
    } ind_none = { ".", A_NORMAL },
      ind_prog = { ">", A_NORMAL },
      ind_menu = { "*", A_NORMAL };

static char *get_wd ()
{
    static	char	wd[400];

    (void)getcwd (wd, 400);
    return (wd);
    } /* get_wd */

static char *internal_env[100] = {
    "/tmp/cm000000",
    "/tmp/cm000001",
    "/tmp/cm000002",
    "/tmp/cm000003",
    "/tmp/cm000004",
    "/tmp/cm000005",
    "/tmp/cm000006",
    "/tmp/cm000007",
    "/tmp/cm000008",
    "/tmp/cm000009",
    };

/* n:DBPATH
 * v:/p/probev/db
 * e:DBPATH=/p/probev/db
 */
static void Putenv (char *n, char *v)
{
    auto	int	l;
    auto	char	*e;
    
    l = strlen (n) + 4 + (v ? strlen (v) : 0);
    e = (char *)calloc (l, sizeof (char));
    if (v)
	(void)sprintf (e, "%s=%s", n, v);
    else {
	(void)strcpy (e, n);
	v = e; e = n; n = v;
	while (*v && *v != '=')
	    v++;
	*v++ = (char)0;
	}
    if (dflg)
	(void)fprintf (stderr, "Putenv (%s, %s); [%s]\n", n, v, e);
    putenv (e);
    } /* Putenv */

static void repaint (WINDOW *w)
{
    WINDOW	*r;
    int	begx, maxx, begy, maxy;

    getbegyx (w, begy, begx);
    getmaxyx (w, maxy, maxx);

    r = newwin (maxy, maxx, begy, begx);
    wclear   (r);
    wrefresh (r);
    delwin   (r);
    touchwin (w);
    wrefresh (w);
    } /* repaint */

static void World (int w)
{
    if (w) {	/* Clear screen for command mode	*/
	if (!wworld)
	    wworld = newwin (LINES, COLS, 0, 0);
	wclear (wworld);
	wrefresh (wworld);
	resetterm ();
	}
    else {	/* Restore screen to menu-mode		*/
	fixterm ();
	if (wworld) {
	    wclear (wworld);
	    wrefresh (wworld);
	    delwin (wworld);
	    }
	wworld = (WINDOW *)0;
	}
    } /* World */

static void FreeMenu (int n)
{
    Menu	*mp = menus[n];
    int	i;

    if (mp) {
	if (dflg)
	    (void)fprintf (stderr, "FreeMenu (%d)\n", n);
	for (i = 0; i < LLINE; i++) {
	    if (mp->line[i]) {
		(void)free ((char *)(mp->line[i]));
		mp->line[i] = (Menuline *)0;
		}
	    }
	for (i = 0; mp->env[i][0]; i++) {
	    Putenv (mp->env[i][0], mp->env[i][1]);
	    (void)free (mp->env[i][0]);
	    (void)free (mp->env[i][1]);
	    mp->env[i][0] = NIL;
	    }
	(void)free ((char *)mp);
	menus[n] = (Menu *)0;
	}
    } /* FreeMenu */

static void str_compress (char *s, int l)
{
    char	*d;
    int	j;

    d = s;
    j = 0;
    s[l - 1] = (char)0;
    while (isspace ((byte)s[j]))	/* Skip leading whitespace */
	j++;
    while (s[j]) {
	if (s[j] == '\\') {
	    switch (s[++j]) {
		/* \7, \13, \322, \b, \e, \f, \n, \r, \t */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		    s[j] -= '0';
		    if (isodigit ((byte)s[j + 1])) {
			j++;
			s[j] += s[j - 1] * 8 - '0';
			}
		    if (isodigit ((byte)s[j + 1])) {
			j++;
			s[j] += s[j - 1] * 8 - '0';
			}
		    break;
		case 'b': s[j] = '\b'; break;
		case 'e': s[j] = '\33'; break;
		case 'f': s[j] = '\f'; break;
		case 'n': s[j] = '\n'; break;
		case 'r': s[j] = '\r'; break;
		case 't': s[j] = '\t'; break;
		}
	    }
	else
	if (isspace ((byte)s[j])) {
	    while (isspace ((byte)s[j + 1]))
		j++;
	    s[j] = (char)0;
	    }
	*d++ = s[j++];
	}
    /* Clear rest of compressed cmd-line */
    (void)memset (d, 0L, l - (d - s));
    } /* str_compress */

static Menu *get_men (char *nm, int cm)
{
    static	char	line[2048];
    Menu	*mp = (Menu *)0;
    Menuline *ml;
    int	i, j;
    char	*m = line, *s, *ids = valid_ids;
    FILE	*F;

    if (dflg)
	(void)fprintf (stderr, "get_men (\"%s\", %d)\n", nm, cm);
    if (nflg && cm) {
	/* Trace back in chain if already in here */
	if (dflg)
	    (void)fprintf (stderr, "no-loop test on %d\n", cm);
	for (i = cm - 1; i >= 0; i--) {
	    if (strncmp (menus[i]->fname, nm, 128) == 0)
		break;
	    }
	if (i >= 0) {
	    for (j = cm - 1; j > i; j--)
		FreeMenu (j);
	    if (dflg) {
		(void)fprintf (stderr,
		    "Return to menu %d (Current = %d)\n", i, cm);
		}
	    return (menus[curm = i]);
	    }
	}

    if (dflg) (void)fprintf (stderr, "get_men opens '%s'\n", nm);
    if ((F = fopen (nm, "r")) == NULL) {
	perror (nm);
	return (mp);
	}
    if ((mp = (Menu *)calloc (1, sizeof (Menu))) == (Menu *)0)
	return (mp);
    
    (void)strncpy (mp->fname, nm, 128);
    mp->aliascnt = 0;
    while (fgets (m, 2048, F)) {
	while ((i = strlen (m)) && m[i - 2] == '-' &&
	     fgets (m + i - 2, 2048 - i - 1, F));

	if (dflg)
	    (void)fprintf (stderr, "<<%s>>", m);
	if (!*m || *m == '\n' || *m == '#')
	    continue;

	i = 0;
	if (islower ((byte)*m))
	    *m -= 32;
	if (*m == 'S')
	    *m = 'Q';
	ids[LLINE] = *m;
	while (ids[i] != *m)
	    i++;
	if (i <= QLINE) {
	    ml = (Menuline *)calloc (1, sizeof (Menuline));
	    if (!ml) {
		FreeMenu (cm);
		return ((Menu *)0);
		}
	    mp->line[i] = ml;
	    /* id[=alias]:label:command:active */
	    ml->id = *m;
	    if (m[1] == '=') {
		ml->alias = toupper (m[2]);
		mp->aliascnt++;
		j = 4;
		}
	    else {
		ml->alias = toupper (m[0]);
		j = 2;
		}
	    sscanf (m + j, "%[^:]:%[^:]:%d:%s\n",
		ml->label, ml->cmd, &ml->active, ml->helpf);
	    if (ml->active & ACTV_WAIT)
		ml->active |= ACTV_ACT;
	    if (ml->active & ACTV_ACT)
		ml->active |= ACTV_DISP;

	    /* Clip tabs/spaces at end of label */
	    s = ml->label;
	    j = strlen (s);
	    while (j && isspace ((byte)s[j - 1]))
		j--;
	    s[j] = (char)0;

	    /* Compress, optionally split, commandline */
	    str_compress (ml->cmd, LCMDMAX);
	    }
	}
    for (mp->idx = QLINE, i = 1; i <= MLINES; i++) {
	if (mp->line[i] && (mp->line[i]->active & ACTV_ACT)) {
	    mp->idx = i;
	    break;
	    }
	}
    if (cm)
	strncpy (mp->line[QLINE]->label,
		 menus[cm - 1]->line[HLINE]->label, LWIDTH);
    fclose (F);
    return (curm = cm, mp);
    } /* get_men */

static int extra_fkey (WINDOW *);
static int unmap (WINDOW *w, int c)
{
    switch (c) {
	case 0x01:          return (extra_fkey (w));
	case KEY_F (15):    return (KEY_UP);
	case KEY_F (16):    return (KEY_DOWN);
	case KEY_F (14):
	case 0x7F:          return (KEY_LEFT);
	case KEY_ENTER:     return ('\r');
	}
    return (c);
    } /* unmap */

static int getmapped (WINDOW *w)
{
    auto	int	c;

    c = unmap (w, wgetch (w));
    return (c);
    } /* getmapped */

static int extra_fkey (WINDOW *w)
{
    int	x;

    x = wgetch (w);
    if (Wflg) {	/* We know about <Funct> keys for Wyse-60 */
	if (x >= 'I' && x <= 'O') {       /* Function keys 10 - 16 */
	    if (wgetch (w) == 0x0D)
		return (unmap (w, KEY_F (10 + x - 'I')));
	    return (1);
	    }
	if (x >= '`' && x <= 'o') {       /* Shifted Function keys 1 - 16 */
	    if (wgetch (w) == 0x0D)
		return (unmap (w, KEY_F (17 + x - '`')));
	    }
	/* wungetch ? */
	return (1);
	}
    if (x == '.') {	/* Emulator function keys */
	x = wgetch (w);
	if (isdigit ((byte)x))
	    return (unmap (w, KEY_F (x - '0')));
	if (isupper ((byte)x))
	    return (unmap (w, KEY_F (10 + x - 'A')));
	return (1);
	}
    if (x == 'e') {	/* Emulator editing keys */
	x = wgetch (w);
	switch (x) {
	    case 'i':	return (unmap (w, KEY_IC));
	    case 'I':	return (unmap (w, KEY_IL));
	    case 'd':	return (unmap (w, KEY_DC));
	    case 'D':	return (unmap (w, KEY_DL));
	    case 'c':	return (unmap (w, KEY_EOL));
	    case 'C':	return (unmap (w, KEY_EOS));
	    }
	return (1);
	}
    if (x == 'c') {	/* Emulator cursor keys */
	x = wgetch (w);
	switch (x) {
	    case 'P':	return (unmap (w, KEY_PPAGE));
	    case 'b':	return (unmap (w, KEY_BACKSPACE));
	    case 'd':	return (unmap (w, KEY_DOWN));
	    case 'h':	return (unmap (w, KEY_HOME));
	    case 'l':	return (unmap (w, KEY_LEFT));
	    case 'p':	return (unmap (w, KEY_NPAGE));
	    case 'r':	return (unmap (w, KEY_RIGHT));
	    case 'u':	return (unmap (w, KEY_UP));
	    }
	return (1);
	}
    return (1);
    } /* extra_fkey */

static void dis_fkeys (int x)
{
    static   char   *keys[2][8] = {
	{ "Vrg mnu", "Keuze",  "", "", "Stop", "Help", "Info", "Verfris" },
	{ "Prev",    "Select", "", "", "Exit", "Help", "Info", "Redraw"  }};
    int    i;

    if (!x) {
	if (fkeyw) {
	    wclear (fkeyw);
	    wrefresh (fkeyw);
	    delwin (fkeyw);
	    fflush (stdout);
	    }
	fkeyw = (WINDOW *)0;
	return;
	}
    if (!fkeyw)
	fkeyw = newwin (1, 80, LINES - 1, 0);
    for (i = 0; i < 8; i++) {
	wmove    (fkeyw, 0, 10 * i);
	wattrset (fkeyw, A_REVERSE);
	wprintw  (fkeyw, "%1d %-7.7s", i + 1,
	    i == 5 && !(menus[curm]->line[menus[curm]->idx]->helpf[0]) ?
	    "" : keys[lang][i]);
	wattrset (fkeyw, A_NORMAL);
	}
    wnoutrefresh (fkeyw);
    } /* dis_fkeys */

static void clear_all ()
{
    if (dflg)
	(void)fprintf (stderr, "clear_all\n");
    clear ();
    wnoutrefresh (stdscr);
    dis_fkeys (0);
    clear ();
    refresh ();
    fflush (stdout);
    endwin ();
    if (Wflg)
	(void)write (1, "\033*", 2);
    else
    if (Vflg)
	(void)write (1, "\233H\233\62J", 5);
    else
    if (Xflg)
	(void)write (1, "\33[H\33[2J", 7);
    usleep (100);
    } /* clear_all */

static void uwait ()
{
    int i;
    char x;

    flushinp ();
    (void)write (1, "\r\n", 2);
    if (Wflg) (void)write (1, "\033Gt", 3);		else
    if (Vflg) (void)write (1, "\233\61;\67m", 5);	else
    if (Xflg) (void)write (1, "\33[31m", 5);
    (void)write (1, lang ? "Return > " : "Enter > \0", 9);
    if (Wflg) (void)write (1, "\033G0", 3);		else
    if (Vflg) (void)write (1, "\233\60m", 3);		else
    if (Xflg) (void)write (1, "\33[0m", 4);
    raw ();
    noecho ();
    while ((i = read (0, &x, 1)) <= 0 || (x != 0x0D && x != 0x0A)) {
	if (i <= 0)  
	    usleep (10000);
	}
    } /* uwait */

static void u_box (WINDOW *w)
{
    if (dflg)
	(void)fprintf (stderr, "u_box (0x%08x)\n", (unsigned)w);
    wnoutrefresh (w);
#ifdef IBM
    if (Wflg) {
	(void)wborder (w, '\263','\263','\304','\304','\332','\277','\300','\331');
	return;
	}
#endif
    (void)box (w, 0, 0);
    } /* u_box */

static char *expand_arg (char *s)
{
    static   char buf[1024], env[256];
    int  i = 0, j = 0, n;
    char *d = buf, *e;

    if (!strchr (s, '$') && !strchr (s, '%'))
	return (s);

    if (aexp[expidx]) {
	free (aexp[expidx]);
	aexp[expidx] = NIL;
	}
    do {
	while (s[i] && (s[i] != '$' && s[i] != '%'))
	    d[j++] = s[i++];
	if (!s[i])
	    break;
	if (s[i] == '%') {
	    if (!isxdigit ((byte)s[i + 1]) || s[i + 2] != '%') {
		d[j++] = s[i++];
		continue;
		}
	    n = s[i + 1];
	    i += 3;
	    n = n - (isdigit ((byte)n) ? '0' : (isupper ((byte)n) ? 'A' : 'a') - 10);
	    e = answers[n];
	    }
	else {
	    e = env;
	    while (s[++i] && isident[(unsigned)s[i]])
		*e++ = s[i];
	    *e = (char)0;
	    if (!(e = getenv (env))) {	/* $TMPFIL	*/
		if (isdigit ((byte)*env)) {	/* $8		*/
		    n = atoi (env);
		    if (n >= 0 && n < 100)
			e = internal_env[n];
		    else
			e = NIL;
		    }
		}
	    }
	if (e)
	    while (*e)
		d[j++] = *e++;
	} while (s[i]);

    d[j] = (char)0;
    if ((e = (char *)malloc (j + 1))) {
	aexp[expidx++] = e;
	(void)strcpy (e, d);
	}
    return (e);
    } /* expand_arg */

static void mhelp (Menuline *ml)
{
    static	char	s[HWIDTH], *c;
    static	WINDOW	*h = (WINDOW *)0;
    int	i, j;
    FILE	*f;
    long	sz;

    if (!ml) {
	if (h) {
	    wclear (h);
	    wrefresh (h);
	    delwin (h);
	    }
	h = (WINDOW *)0;
	return;
	}
    c = expand_arg (ml->helpf);
    f = fopen (c, "r");
    if (c != ml->helpf)
	expidx--;
    if (f == (FILE *)0)
	return;
    (void)fseek (f, 0, SEEK_END);
    sz = ftell (f);
    rewind (f);
    if (!h) {
	h = newwin (LINES - 1, HWIDTH, 0, 80 - HWIDTH);
	(void)keypad    (h, keypad_mode);
	(void)scrollok  (h, FALSE);
	(void)idlok     (h, FALSE);
	(void)intrflush (h, FALSE);
	}
    wmove    (h, 1, 1);
    wattrset (h, _A_STANDOUT);
    wprintw  (h, " %-*.*s ", HTEXT, HTEXT, ml->label);
    wattrset (h, A_NORMAL);
    do {
	i = 3;				/* HTEXT + 2: "\n\0" */
	while (i < LINES - 2 && (c = fgets (s, HTEXT + 2, f))) {
	    j = strlen (s) - 1;
	    if (s[j] == '\n')
		s[j] = (char)0;
	    if ((j == 1 && s[0] == '\f') ||
		(j == 2 && s[0] == '^' && s[1] == 'L') ||
		(j == 2 && s[0] == '^' && s[1] == 'l') ||
		(j == 3 && s[0] == '.' && s[1] == 'P' && s[2] == 'A'))
		break;
	    mvwprintw (h, i++, 2, "%-*.*s", HTEXT, HTEXT, s);
	    wclrtoeol (h);
	    }
	while (i < LINES - 2) {
	    wmove (h, i++, 2);
	    wclrtoeol (h);
	    }
	if (ftell (f) == sz)
	    c = NIL;
	wmove     (h, i, 1);
	wclrtobot (h);
	wmove     (h, 1, HWIDTH - 3);
	if (c) {
	    wattrset  (h, _A_STANDOUT | A_BLINK | ind_menu.att);
	    waddstr   (h, ind_menu.str);
	    }
	else {
	    wattrset  (h, _A_STANDOUT);
	    waddstr   (h, " ");
	    }
	wattrset  (h, A_NORMAL);
	wmove     (h, LINES - 3, HWIDTH - 2);
	u_box     (h);
	wrefresh  (h);
	do {
	    i = getmapped (h);
	    switch (i) {
		case KEY_F (7):
		    mvwprintw (h, 2, 2, "%s/%s ...", get_wd (), ml->helpf);
		    wrefresh  (h);
		    sleep     (3);
		    wmove     (h, 2, 2);
		    wclrtoeol (h);
		    wmove     (h, LINES - 3, HWIDTH - 2);
		    u_box     (h);
		    wrefresh  (h);
		    i = 0;
		    break;
	      }
	    } while (i == 0);
	} while (c && !(i == 'q' || i == KEY_F (1) || i == KEY_F (5)));
    } /* mhelp */

static int exec_men (Menuline *, int);
static int inp_nstr (WINDOW *window, char *p_str, int cnt, int silent, int dl, int ef)
{
    int	c;
    int	i, j, n;
    int	e;	/* 0 = insert, 1 = replace */
    char	*s = p_str;
    int	action = 0, x, y;
    struct IND	*ind;

    i = 0;
    n = strlen (s);
    if ((e = ef)) {
	i = n;
	getyx     (window, y, x);
	wmove     (window, y, x + i);
	wrefresh  (window);
	}
    while ((i < cnt) && ((c = getmapped (window)) != '\r' && c != '\n')) {
	getyx (window, y, x);
	switch (c) {
	    case KEY_F (1):
		return (-2);

	    case KEY_F (2):
		return (action ? i : -1);

	    case KEY_F (3):
	    case KEY_F (4):
		break;

	    case KEY_F (5):
		clear_all ();
		exit (0);

	    case KEY_F (6):
		if (window == dbox && ml_shell.helpf[0]) {
		    (void)mhelp (&ml_shell);
		    touchwin (stdscr);
		    wnoutrefresh (stdscr);
		    touchwin (window);
		    wrefresh (window);
		    }
		break;

	    case KEY_F (7):
		break;

	    case KEY_F (8):
		repaint (window);
		break;

	    case KEY_F (9):
	    case KEY_F (10):
	    case KEY_F (11):
	    case KEY_F (12):
	    case KEY_F (13):
	    case KEY_F (14):
	    case KEY_F (15):
	    case KEY_F (16):
	    case KEY_F (17):
	    case KEY_F (18):
	    case KEY_F (19):
	    case KEY_F (20):
	    case KEY_F (21):
	    case KEY_F (22):
		break;

	    case '!':
	    case KEY_F (23):	/* <SHIFT><F7>,  <FUNCT>f	*/
		if (aflg) {
		    mvwprintw (window, 2, 2, "Command > ");
		    wrefresh (window);
		    memset (ml_shell.cmd, 0L, LCMDMAX);
		    aflg = 0;	/* prevent loop !!! */
		    if (inp_nstr (window, ml_shell.cmd,
				  LCMDMAX, 0, 66, 0) > 0) {
			str_compress (ml_shell.cmd, LCMDMAX);
			ml_shell.active = 2;
			(void)exec_men (&ml_shell, 0);
			}
		    wnoutrefresh (stdscr);
		    wnoutrefresh (fkeyw);
		    /*
		    touchwin (window);
		    */
		    wmove (window, 2, 2);
		    wclrtoeol (window);
		    u_box (window);
		    wmove (window, y, x);
		    wrefresh (window);
		    aflg = 1;
		    }
	    case KEY_F (24):
	    case KEY_F (25):
	    case KEY_F (26):
	    case KEY_F (27):
	    case KEY_F (28):
	    case KEY_F (29):
	    case KEY_F (30):
	    case KEY_F (31):
	    case KEY_F (32):
		break;

	    case KEY_IC:
	    case KEY_EIC:	/* toggle insert mode */
		e ^= 1;
		break;

	    case KEY_EOL:
		n = i;
		s[n] = (char)0;
		break;

	    case KEY_LEFT:
	    case KEY_BACKSPACE:
	    case '\b':
		if (i <= 0) {
		    flash ();
		    break;
		    }
		if (!silent)
		    wmove (window, y, --x);
		i--;
		if (e)
		    break;
		/* FALL THROUGH */
	    case KEY_DC:
		if (!s[i]) {
		    flash ();
		    break;
		    }
		if (!silent)
		    wdelch (window);
		for (j = i; j < n; s[j] = s[j + 1], j++);
		n--;
		break;

	    case KEY_RIGHT:
		if (!s[i]) {
		    flash ();
		    break;
		    }
		if (!silent)
		    wmove (window, y, ++x);
		i++;
		break;

	    case 'q':
		if (!action)
		    return (-2);
		/* FALL THROUGH */
	    default:
		if (!e) {
		    if (!action)
			s[n = 0] = (char)0;
		    for (j = ++n; j > i; j--)
			s[j] = s[j - 1];
		    }
		s[i++] = c;
		if (i > n)
		    s[n = i] = (char)0;
		if (!silent) {
		    if (e)
			waddch (window, c);
		    else
			winsch (window, c);
		    }
		x++;
		break;
	    }
	action++;
	if (!silent) {
	    ind = e ? &ind_prog : &ind_menu;
	    wmove     (window, y, x - i - 2);
	    wattrset  (window, ind->att);
	    waddstr   (window, ind->str);
	    wattrset  (window, A_NORMAL);
	    wmove     (window, y, x + strlen (s) - i);
	    wclrtoeol (window);
	    u_box     (window);
	    wmove     (window, y, x);
	    wrefresh  (window);
	    }
	}
    return (action ? n : -1);
    } /* inp_nstr */

static void logo ()
{
    static	char	Wpos[8] = "\033=  ", Vpos[24];
    auto	int	curx, cury;

    getyx (stdscr, cury, curx);
    if (Wflg) {
	(void)write (1, "\33=!/\33cE\33Gt\332\33G0\33cD\b", 18);
	Wpos[2] = ' ' + cury;
	Wpos[3] = ' ' + curx;
	(void)write (1, Wpos, 4);
	}
    else
    if (Vflg) {
	(void)write (1, "\233\62;16H\233\67m\217\61\233\60m", 14);
	(void)sprintf (Vpos, "\233%02d;%03dH", cury + 1, curx + 1);
	(void)write (1, Vpos, 8);
	}
    } /* logo */

static int dis_men (Menu *mp)
{
    Menuline	*ml;
    int		i;
    int		a, x;
    struct IND	*ind;

    if (dflg)
	(void)fprintf (stderr, "dis_men (0x%08x, %d)\n", (unsigned)mp, curm);

    attrset (_A_STANDOUT);
    mvprintw (1, 1, " %8.8s%*s%-*.*s%*d ", scurtim, LSTART - 6, "",
	LWIDTH - 4, LWIDTH - 4, mp->line[0]->label,
	COLS - LWIDTH - LSTART - 2, curm + 1);
    attrset (A_NORMAL);

    for (i = 1; i < QLINE; i++) {
	ml = mp->line[i];
	move (LINEOFF + i, LSTART);
	attrset (A_NORMAL);
	if (ml && (ml->active & ACTV_DISP)) {
	    if (ml->active & ACTV_ACT) {
		printw ("%c ", ml->alias ? ml->alias : valid_ids[i]);
		ind = strcmp (ml->cmd, "M") ? &ind_prog : &ind_menu;
		attrset (ind->att);
		addstr (ind->str);
		addch (' ');
		}
	    else {
		attrset (ind_none.att);
		addstr (ind_none.str);
		addstr ("   ");
		}
	    if (i == mp->idx)
		attrset (_A_STANDOUT);
	    else {
		char c;

		a = A_NORMAL;
		if ((Wflg | Vflg | Xflg) && !(ml->active & ACTV_ACT)) {
		    for (x = 0; (c = ml->cmd[x]); x++) {
			switch (c) {
			    case 'S': a |= A_STANDOUT;  break;
			    case 'R': a |= A_REVERSE;   break;
			    case 'D': a |= A_DIM;       break;
			    case 'U': a |= A_UNDERLINE; break;
			    case 'B': a |= A_BLINK;     break;
			    }
			}
		    }
		attrset (a);
		}
	    printw ("%-*.*s", LWIDTH - 4, LWIDTH - 4, ml->label);
	    attrset (A_NORMAL);
	    }
	else {
	    addstr (ind_none.str);
	    attrset (A_NORMAL);
	    clrtoeol ();
	    }
	}

    if (uflg) {
	auto char *uname = getlogin ();
	attrset  (A_DIM);
	mvprintw (LINES - 3, 80 - strlen (uname) - 2, uname);
	attrset  (A_NORMAL);
	}

    mvprintw (LINES - 4, LSTART, "S ");
    ind = curm ? &ind_menu : &ind_prog;
    attrset (ind->att);
    addstr (ind->str);
    addch (' ');
    if (mp->idx == QLINE)
	attrset (_A_STANDOUT);
    else
	attrset (A_NORMAL);
    printw ("%-*.*s", LWIDTH - 4, LWIDTH - 4, mp->line[QLINE]->label);
    attrset (A_NORMAL);
    mvprintw (2, 2, "%77s", "");

    move (mp->idx == QLINE ? LINES - 4 : mp->idx + LINEOFF, (LSTART - 2));
    u_box (stdscr);
    dis_fkeys (1);
    refresh ();
    logo ();
    return (1);
    } /* dis_men */

/* ARGSUSED */
static void put_time (int sig)
{
    auto	int	x, y;
    auto	time_t	tm;

    getyx (stdscr, y, x);
    tm = time ((time_t *)0);
    (void)strftime (scurtim, 16, "%H:%M:%S", localtime (&tm));
    attrset  (_A_STANDOUT);
    mvprintw (1, 2, "%s", scurtim);
    attrset  (A_NORMAL);
    move (y, x);
    refresh ();
    signal (SIGALRM, put_time);
    alarm (1);
    } /* put_time */

static Menuline *choose (Menu *mp)
{
    int i, c;

    if (dflg)
	(void)fprintf (stderr, "choose (0x%08x)\n", (unsigned)mp);
    touchwin (fkeyw);
    wrefresh (fkeyw);
    logo ();
    do {
	if (Key_Home) {
	    Key_Home--;
	    c = 's';
	    }
	else {
	    if (tflg) {
		signal (SIGALRM, put_time);
		alarm (4);
		do {
		    c = getmapped (stdscr);
		    } while (c < 0);
		signal (SIGALRM, SIG_IGN);
		alarm (0);
		(void)strcpy (scurtim, "        ");
		}
	    else
		c = getmapped (stdscr);
	    }
	if (c < 0x100 && islower ((byte)c))
	    c = c - 'a' + 'A';

	if (mp->aliascnt) {
	    for (i = 0; i < QLINE; i++) {
		if ( mp->line[i] && (mp->line[i]->active & ACTV_ACT) &&
		     mp->line[i]->alias && mp->line[i]->alias == c) {
		    mp->idx = i;
		    c = KEY_F (2);
		    }
		}
	    }

	switch (c) {
	    case 'J':
	    case '\n':
	    case '\r':
	    case KEY_DOWN:
		c = 'J';
		i = mp->idx == QLINE ? 1 : mp->idx + 1;
		while (i < QLINE &&
		      (!mp->line[i] || !(mp->line[i]->active & ACTV_ACT)))
		    i++;
		mp->idx = i;
		break;

	    case 'K':
	    case KEY_UP:
	    case KEY_F (12):
	    case 21:
		i = mp->idx - 1;
		while (i > 0 && (!mp->line[i] || !(mp->line[i]->active & ACTV_ACT)))
		    i--;
		if (i)
		    mp->idx = i;
		else
		    mp->idx = QLINE;  /* WRAP */
		break;

	    case KEY_HOME:
		if (curm) {
		    Key_Home = curm - 1;
		    return ((Menuline *)0);
		    }
		break;

	    case KEY_F (2):
		c = 0x0D;
		if (mp->idx != QLINE)
		    break;
		/* FALLTHROUGH */
	    case KEY_F (1):
		if ( !curm && getenv ("F1EXIT") &&
		     strchr ("NnFf", *getenv ("F1EXIT"))) {
		    c = 0x00;
		    break;
		    }
		/* FALLTHROUGH */
	    case 4:       /* CTRL-D */
	    case 'S':
		if (curm)
		    return ((Menuline *)0);
		/* FALLTHROUGH */
	    case KEY_F (5):
	    case 'Q':
		clear_all ();
		exit (0);

	    case 'A':
	    case 'B':
	    case 'C':
	    case 'D':
	    case 'E':
	    case 'F':
		c -= 'A' - ':';
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		if (mp->aliascnt) {
		    flash ();
		    break;
		    }
		i = c - '0' + 1;
		if (mp->line[i] && (mp->line[i]->active & ACTV_ACT)) {
		    mp->idx = i;
		    c = 0x0D;
		    }
		break;

	    case KEY_F (6):
		mhelp (mp->line[mp->idx]);
		break;

	    case KEY_F (7):
	    case '':
		mvprintw (2, 2, "%s:%s ...", get_wd (), menus[curm]->fname);
		refresh ();
		sleep (3);
		break;

	    case 0x86:	/* <CTRL><F7> */
		if (!aflg)
		    break;
		move (2, 2);
		i = 0;
		do {
		    if (mp->line[mp->idx]->cmd[i])
			addch (mp->line[mp->idx]->cmd[i++]);
		    if (!mp->line[mp->idx]->cmd[i]) {
			addch (' ');
			i++;
			}
		    } while (i < 73 && mp->line[mp->idx]->cmd[i]);
		addstr ("...");
		refresh ();
		sleep (3);
		break;

	    case 0x96:	/* <SHFT><CTRL><F7> */
		if (!aflg)
		    break;
		mvprintw (2, 2, "%s [%s] ...",
		    getenv ("ACLENV"), getenv ("DBPATH"));
		refresh ();
		sleep (3);
		break;

	    case KEY_F (8):
	    case '':
		clear ();
		refresh ();
		break;

	    case KEY_F (9):
	    case KEY_F (10):
	    case KEY_F (11):
	    case KEY_F (13):
	    case KEY_F (14):
	    case KEY_F (15):
	    case KEY_F (16):
	    case KEY_F (17):
	    case KEY_F (18):
		continue;

	    case KEY_F (19):	/* <SHIFT><F3>,  <FUNCT>b	*/
		if (aflg) {
		    (void)strcpy (ml_shell.cmd, "sh");
		    ml_shell.cmd[3] = (char)0;
		    ml_shell.active = 1;
		    (void)exec_men (&ml_shell, 0);
		    break;
		    }
	    case KEY_F (20):	/* <SHIFT><F4>,  <FUNCT>c	*/
		if (aflg) {
		    (void)strcpy (ml_shell.cmd, "csh");
		    ml_shell.cmd[4] = (char)0;
		    ml_shell.active = 1;
		    (void)exec_men (&ml_shell, 0);
		    break;
		    }
	    case KEY_F (21):
	    case KEY_F (22):
		continue;

	    case '!':
	    case KEY_F (23):	/* <SHIFT><F7>,  <FUNCT>f	*/
		if (aflg) {
		    mvprintw (2, 2, "Command > ");
		    refresh ();
		    memset (ml_shell.cmd, 0L, LCMDMAX);
		    aflg = 0;	/* prevent loop !!!	*/
		    if (inp_nstr (stdscr, ml_shell.cmd,
				  LCMDMAX, 0, 66, 0) > 0) {
			str_compress (ml_shell.cmd, LCMDMAX);
			ml_shell.active = 2;
			(void)exec_men (&ml_shell, 0);
			}
		    aflg = 1;
		    break;
		    }
	    case KEY_F (24):
	    case KEY_F (25):
	    case KEY_F (26):
	    case KEY_F (27):
		continue;

	    case KEY_F (28):	/* <SHIFT><F12>, <FUNCT>k	*/
		if (aflg) {
		    (void)strcpy (ml_shell.cmd, "ksh");
		    ml_shell.cmd[4] = (char)0;
		    ml_shell.active = 1;
		    (void)exec_men (&ml_shell, 0);
		    break;
		    }
	    case KEY_F (29):
		continue;

	    case KEY_F (30):	/* <SHIFT><F14>, <FUNCT>m	*/
		if (aflg) {
		    mvprintw (2, 2, "Menu > ");
		    refresh ();
		    (void)memset (ml_shell.cmd, 0L, LCMDMAX);
		    (void)strcpy (ml_shell.cmd, "M ");
		    if (inp_nstr (stdscr, ml_shell.cmd + 2,
				  LCMDMAX - 2, 0, 66, 0) > 0) {
			str_compress (ml_shell.cmd, LCMDMAX);
			ml_shell.active = 2;
			return (&ml_shell);
			}
		    break;
		    }
	    case KEY_F (31):
	    case KEY_F (32):
		continue;

	    default:
		if (vflg) {
		    mvprintw (2, 2, "key %d (0x%04X) unsup", c, c);
		    refresh ();
		    sleep (3);
		    }
		flash ();
	    }
	dis_men (mp);
	} while (c != 0x0D);
    return (mp->line[mp->idx]);
    } /* choose */

static void dis_qkeys ()
{
    static	char	*keys[2][8] = {
	{ "Menu", "Keuze",  "", "", "Stop", "Help", "", "Verfris" },
	{ "Menu", "Select", "", "", "Exit", "Help", "", "Redraw"  }};
    int	i;

    if (!fkeyw)
	fkeyw = newwin (1, 80, LINES - 1, 0);
    for (i = 0; i < 8; i++) {
	wmove    (fkeyw, 0, 10 * i);
	wattrset (fkeyw, A_REVERSE);
	wprintw  (fkeyw, "%1d %-7.7s", i + 1,
	    i == 5 && !ml_shell.helpf[0] ? "" : keys[lang][i]);
	wattrset (fkeyw, A_NORMAL);
	}
    wnoutrefresh (fkeyw);
    } /* dis_qkeys */

/* ?#[?][,c]quest[,default[,prefix[,options[,helpfile]]]]
 * c can be an alternate separator instead of ,
 * answer can be entered after prompt quest is displayed
 * answer is set to default on return
 * answer is prefixed with prefix if filled
 * answer can be edited if option E (editable)
 * answer will not be echoed if option S (silent)
 * answer will be required (must be filled) if option R (required)
 * answer will be comment only if option C (comment)
 * answer, if filled, must exist as directory if option d
 * answer, if filled, must exist if option e
 * answer, if filled, must exist as file if option f
 * obviously options C and R are mutualy exclusive
 * helpfile will be displayed (and paged) on F6 if exists
 */
static char *dialogue_box (char *question, char *header)
{
    static	struct stat reqstat;
    static	char	quest[1024], def_ans[1024];
    WINDOW	*db = dbox;
    char	*q = quest, *d, *o = "";
    char	*a = &answers[dialogue_idx][0];
    int	ql, al;
    char	sep = ',', *opt, *hlpf, reqtype = (char)0;
    int	dl = 0, ol = 0, optl = 0;
    int	silent = 0, required = 0, editable = 0;

    if (!question) {
	if (db)
	    (void)delwin (db);
	dbox = (WINDOW *)0;
	return (NIL);
	}
    if (!db) {
	if (wworld) {	/* Put in a known state	*/
	    World (0);
	    touchwin (stdscr);
	    wnoutrefresh (stdscr);
	    }
	dbox = newwin (LINES - 5, 80, 4, 0);
	db = dbox;
	for (ql = 0; ql < 16; *answers[ql++] = (char)0);
	(void)keypad    (db, keypad_mode);
	(void)scrollok  (db, FALSE);
	(void)idlok     (db, FALSE);
	(void)intrflush (db, FALSE);
	}
    u_box (db);

    (void)strncpy (q, question, 80);
    *ml_shell.helpf = (char)0;
    /* Backward compatible, now in options */
    if (*q == '?') {	/* SILENT		*/
	q++;
	silent = 1;
	}
    if (*q == ',') {	/* Change separator	*/
	q++;
	sep = *q++;
	}
    if ((d = strchr (q, sep))) {
	ql = d++ - q;
	if ((o = strchr (d, sep))) {
	    dl = o++ - d;
	    if ((opt = strchr (o, sep))) {
		ol = opt++ - o;
		if ((hlpf = strchr (opt, sep))) {
		    optl = hlpf++ - opt;
		    (void)strncpy (ml_shell.helpf, hlpf, 127);
		    }
		else
		    optl = strlen (opt);
		for (al = 0; al < optl; al++) {
		    switch (opt[al]) {
			case 'S': silent	= 1;	break;
			case 'R': required	= 1;	break;
			case 'C': required	= -1;	break;
			case 'E': editable	= 1;	break;
			case 'd':
			case 'e':
			case 'f':
			case 's': reqtype = opt[al]; break;
			}
		    }
		}
	    else {
		ol = strlen (o);
		opt = "";
		}
	    }
	else {
	    dl = strlen (d);
	    o = "";
	    }
	}
    else {
	ql = strlen (q);
	d = "";
	}
    for (al = 0; al < ql; al++)
	if (q[al] == '_')
	    q[al] = ' ';
    if (ml_shell.helpf[0]) {
	al = min (ql, LWIDTH);
	(void)strncpy (ml_shell.label, q, al);
	ml_shell.label[al] = (char)0;
	}
    (void)strncpy (def_ans, d, dl);
    d = def_ans;
    if (dl > LWIDTH)	/* sorry, cannot deal with too long defaults */
	dl = LWIDTH;
    d[dl] = (char)0;
    if (strchr (d, '$') || strchr (d, '%')) {
	(void)strcpy (d, expand_arg (d));
	expidx--;
	}
    dl = strlen (d);
    dis_qkeys ();
    wattrset (db, _A_STANDOUT);
    mvwprintw (db, 1, 1, "%27s%-51.51s", "", header);
    wattrset (db, A_NORMAL);
    do {
	do {
	    mvwprintw (db, 3 + dialogue_idx, 2, "%-23.*s %s %.*s",
		min (required == -1 ? 76 : 23, ql),
		q, required < 0 ? " " : ">", dl, d);
	    wmove (db, 3 + dialogue_idx, 28);
	    wrefresh (db);
	    if (required < 0) {
		*a = (char)0;
		al = 0;
		break;
		}
	    /* q, <F1>   ???	*/
	    (void)strcpy (a, silent ? "" : d);
	    if ((al = inp_nstr (db, a, 52, silent, dl, editable)) == -2) {
		(void)strcpy (a, "q");
		return (a);
		}
	    if (al == -1) {
		strncpy (a, d, dl);
		al = dl;
		a[al] = (char)0;
		}
	    while (al && isspace ((byte)a[al - 1]))
		a[--al] = (char)0;
	    } while (required && al == 0);
	if (*a && ol) {
	    (void)strncpy (d, o, ol);
	    d[ol] = (char)0;
	    (void)sprintf (d, "%s%s",
		(strchr (d, '$') || strchr (d, '%')) ?
		expand_arg (d) : d, a);
	    (void)strcpy (a, d);
	    }
	if (reqtype && *a) {
	    if (stat (a, &reqstat))
		reqstat.st_uid = 1;
	    else {
		reqstat.st_uid = 0;
		switch (reqtype) {
		    case 'e': break;
		    case 'd': if (!S_ISDIR (reqstat.st_mode))
				  reqstat.st_uid = 2;
			      break;
		    case 'f': if (!S_ISREG (reqstat.st_mode))
				  reqstat.st_uid = 3;
			      break;
		    case 's': if (reqstat.st_size <= 0)
				  reqstat.st_uid = 4;
			      break;
		    }
		}
	    if (reqstat.st_uid) {
		static char *reqmsg[2][4] = {
		    { "%s bestaat niet",
		      "%s is geen directory",
		      "%s is geen normaal bestand",
		      "%s is leeg" },
		    { "%s does not exist",
		      "%s is not a directory",
		      "%s is not a regular file",
		      "%s is empty" }};

		flash ();
		mvwprintw (db, 3 + dialogue_idx, 28,
		    reqmsg[lang][reqstat.st_uid - 1], a);
		continue;
		}
	    }
	reqtype = (char)0;
	} while (reqtype);
    wrefresh (db);
    refresh ();
    return (a);
    } /* dialogue_box */

static void execute (char *args[])
{
    auto	int	fd[2], i, f;

    for (i = 0; args[i]; i++) {
	if (strcmp (args[i], "|") == 0) {
	    args[i] = NIL;
	    if (pipe (fd) || (f = fork ()) < 0)
		exit (errno);
	    if (f) {
		(void)close (0);
		(void)dup   (fd[0]);
		(void)close (fd[0]);
		(void)close (fd[1]);
		(void)execute (&args[i + 1]);
		}
	    (void)close (1);
	    (void)dup   (fd[1]);
	    (void)close (fd[0]);
	    (void)close (fd[1]);
	    break;
	    }
	}
    (void)execvp (args[0], args);
    exit (errno);
    } /* execute */

static int new_menu (char *);
static int exec_men (Menuline *ml, int nxtcmd)
{
    char *c, *twd, *std, *err;
    int  i, j, k;
    int  fork_flag, background, stdm = 0, errm = 0;

    if (!ml)
	return (1);

    if (dflg)
	(void)fprintf (stderr, "exec_men (0x%08x, %d)\n", (unsigned)ml, nxtcmd);
    for (i = 0; i < expidx; i++) {
	if (aexp[expidx]) {
	    free (aexp[expidx]);
	    aexp[expidx] = NIL;
	    }
	}
    expidx = 0;
    if (nxtcmd) {
	for (i = 0; args[nxtcmd]; args[i++] = args[nxtcmd++]);
	args[i] = NIL;
	}
    else {
	c = ml->cmd;
	i = 0;
	do {
	    args[i++] = c;
	    while (*c)
		c++;
	    } while (*++c);
	}

    if (strcmp (args[0], "M") == 0)
	return (new_menu (args[1]));

    nxtcmd = 0;

    twd = NIL;
    std = NIL;
    err = NIL;
    dialogue_idx = -1;
    j = 0;
    while (j < i) {
	if (strcmp (args[j], ";") == 0) {
	    nxtcmd = j + 1;
	    break;
	    }
	if (*args[j] == '@') {      /* @: target working directory */
	    twd = &args[j][1];
	    for (k = j + 1; k <= i; k++)
		args[k - 1] = args[k];
	    i--;
	    twd = expand_arg (twd);
	    continue;
	    }
	if (*args[j] == '>') {      /* >: redirected output */
	    std = &args[j][1];
	    if (*std == '>') {
		std++;
		stdm = O_WRONLY | O_CREAT | O_APPEND;
		}
	    else
		stdm = O_WRONLY | O_CREAT | O_TRUNC;
	    for (k = j + 1; k <= i; k++)
		args[k - 1] = args[k];
	    i--;
	    std = expand_arg (std);
	    continue;
	    }
	if (*args[j] == '2' && args[j][1] == '>') { /* 2>: redirected error output */
	    err = &args[j][2];
	    if (*err == '>') {
		err++;
		errm = O_WRONLY | O_CREAT | O_APPEND;
		}
	    else
		errm = O_WRONLY | O_CREAT | O_TRUNC;
	    for (k = j + 1; k <= i; k++)
		args[k - 1] = args[k];
	    i--;
	    err = expand_arg (err);
	    continue;
	    }
	if (j > 0 && *args[j] == '?' && isxdigit ((k = args[j][1]))) {
	    dialogue_idx = k - (isdigit ((byte)k) ? '0' :
			       (isupper ((byte)k) ? 'A' : 'a') - 10);
	    args[j] = dialogue_box (&args[j][2], ml->label);
	    if (strcmp (args[j], "q") == 0)
		return ((void)dialogue_box (0, NIL), 1);
	    if (!*args[j]) {      /* skip blank answers */
		for (k = j + 1; k <= i; k++)
		    args[k - 1] = args[k];
		i--;
		continue;
		}
	    }
	args[j] = expand_arg (args[j]);
	j++;
	}
    if (dialogue_idx >= 0) {
	dialogue_idx = 15;
	if (strcmp ("q", dialogue_box (lang ? "<F1> quit <CR> continue" : "<F1> stop <ENTER> doorgaan", ml->label)) == 0)
	    return ((void)dialogue_box (0, NIL), 1);
	(void)dialogue_box (0, NIL);
	dis_fkeys (1);
	}

    if (j && strcmp (args[j - 1], "&") == 0) {
	background = 1;
	j--;
	}
    else
	background = 0;
    args[j] = NIL;

    if (!args[0])
	return (1);

    if (!wworld)
	World (1);

    if (vflg) {
	for (i = 0; i < j; i++)
	    (void)fprintf (stderr, "%s ", args[i]);
	(void)fprintf (stderr, "\n");
	}
    fork_flag = fork ();
    if (fork_flag == -1)
	perror ("cshmen");
    else {
	if (fork_flag == 0) {
	    signal (2, SIG_DFL);
	    if (twd)
		(void)chdir (twd);
	    if (vflg)
		(void)fprintf (stderr, "cwd: %s\n", get_wd ());
	    if (std) {
		(void)close (1);
		(void)open (std, stdm, 0666);
		}
	    if (err) {
		(void)close (2);
		if (strcmp (err, "&1") == 0)
		    (void)dup (1);
		else
		    (void)open (err, errm, 0666);
		}
	    execute (args);
	    }
	else {
	    if (!background)
		wait ((int *)0);
	    if (nxtcmd)
		exec_men (ml, nxtcmd);
	    else
	    if (ml->active & ACTV_WAIT)
		uwait ();
	    World (0);
	    }
	}
    return (1);
    } /* exec_men */

static void init_men ()
{
    char *tname;

    tname = getenv ("LINES");
    if (!tname || atol (tname) < 12)
	putenv ("LINES=24");
    tname = getenv ("COLUMNS");
    if (!tname || atol (tname) < 40)
	putenv ("COLUMNS=80");

    Vflg = 0;
    Wflg = 0;
    Xflg = 0;
    tname = getenv ("TERM");
    if (strstr (tname, "wy60")) {
	Wflg = 1;
	ind_menu.str = "\257";
	if (strcmp (getenv ("C_GEM"), "0392")) {
	    ind_none.str = "\374";	/* TUN Cannot use it :-( */ 	
	    }
	else {
	    ind_none.str = "\372";
	    }
	}
    else
    if (strstr (tname, "vt320")) {
	Vflg = 1;
	(void)write (1, "\33(B\17\33)0\33~\33.A\33+P", 15);
	ind_menu.str = "\273";	/* Somehow gets lost */
	ind_menu.str = "z";
	ind_menu.att = A_ALTCHARSET;
	ind_none.str = "\376";
	keypad_mode  = FALSE;
	}
    else
    if (strstr (tname, "xterm")) {
	Xflg = 1;
	ind_menu.str = "\273";
	ind_none.str = "\240"/*"\267"*/;
	}
    ml_shell.id	= 'X';
    *ml_shell.label	= (char)0;
    *ml_shell.cmd	= (char)0;
    ml_shell.alias	= (char)0;
    *ml_shell.helpf	= (char)0;

    (void)initscr ();
    (void)delwin (stdscr);
    stdscr = newwin (LINES - 1, 80, 0, 0);
    (void)nonl ();
    (void)cbreak ();
    (void)noecho ();
    (void)keypad (stdscr, keypad_mode);
    setscrreg (0, LINES - 1);
    (void)scrollok (stdscr, FALSE);
    (void)idlok (stdscr, FALSE);
    (void)intrflush (stdscr, FALSE);

    tname = (char *)malloc (strlen (getenv ("PATH")) + 4);
    (void)sprintf (tname, "%s:.", getenv ("PATH"));
    Putenv ("PATH", tname);
    } /* init_men */

static void run_menu ()
{
    Menu	*mp;
    Menuline	*mlp;
    int	i, j, k;
    char	*s, *r, *e;

    mp = menus[curm];
    if (dflg)
	(void)fprintf (stderr, "run_menu (%d) [0x%08x]\n",
	    (unsigned)curm, (unsigned)mp);
    mp->env[0][0] = NIL;
    if (mp->line[0] && (s = mp->line[0]->cmd) && *s) {
	j = 0;
	do {
	    e = s;
	    for (i = 0; s[i] && s[i] != '='; i++);
	    if (!s[i])
		break;
	    s[i] = (char)0;
	    s += i + 1;
	    if (isdigit ((byte)*e)) {
		k = atoi (e) % 1000;
		r = expand_arg (s);
		e = (char *)malloc (strlen (r) + 1);
		internal_env[k] = e;
		(void)strcpy (e, r);
		}
	    else {
		if ((r = getenv (e))) {
		    /* Copy original values */
		    mp->env[j][0] = (char *)malloc (i + 1);
		    (void)strcpy (mp->env[j][0], e);
		    mp->env[j][1] = (char *)malloc (strlen (r) + 1);
		    (void)strcpy (mp->env[j][1], r);
		    mp->env[++j][0] = NIL;
		    }
		Putenv (e, expand_arg (s));
		}
	    s[-1] = '=';
	    for (i = 0; s[i]; i++);
	    s += i + 1;
	    } while (*s);
	}
    do {
	mp = menus[curm];
	if (!dis_men (mp))
	    break;
	mlp = choose (mp);
	if (dflg && mlp)
	    (void)fprintf (stderr, "choose todo mlp 0x%08x (%c '%s' '%s')\n",
		(unsigned)mlp, mlp->id, mlp->label, mlp->cmd);
	if (!mlp)
	    break;
	} while (exec_men (mlp, 0));
    } /* run_menu */

static int new_menu (char *name)
{
    Menu *mp;

    if (dflg)
	(void)fprintf (stderr, "new_menu (\"%s\")\n", name);
    mp = get_men (name, curm + 1);
    if (!mp)
	return (0);
    menus[curm] = mp;
    run_menu ();
    FreeMenu (curm--);
    return (1);
    } /* new_menu */

static int usage (char *pn)
{
    (void)fprintf (stderr,
	"%s: %s [-?] [-a] [-e] [-d] [-v] [-n] [-u] [-t] [-X] [-m dir] [menu]\n",
	lang ? "usage" : "gebruik", pn);
    return (-1);
    } /* usage */

static int help (char *pn)
{
    static char *shelp[2][11] = {
	{ "deze lijst", "engels", "foutopsporing", "breedsprakig",
	  "MENU locatie is dir", "basismenu bestand", "loops mogelijk",
	  "administratief gebruiker", "grafische omgeving", "met klok",
	  "gebruikersnaam in beeld" },
	{ "this list", "english", "debug", "verbose",
	  "base location for MENU is dir", "base menu description file",
	  "enable loops", "administrator", "use X11 GUI", "show time",
	  "show user in menu" }};

    (void)usage (pn);
    (void)fprintf (stderr, "\t-?\t%s\n",       shelp[lang][ 0]);
    (void)fprintf (stderr, "\t-a\t%s\n",       shelp[lang][ 7]);
    (void)fprintf (stderr, "\t-e\t%s\n",       shelp[lang][ 1]);
    (void)fprintf (stderr, "\t-d\t%s\n",       shelp[lang][ 2]);
    (void)fprintf (stderr, "\t-v\t%s\n",       shelp[lang][ 3]);
    (void)fprintf (stderr, "\t-n\t%s\n",       shelp[lang][ 6]);
    (void)fprintf (stderr, "\t-u\t%s\n",       shelp[lang][10]);
    (void)fprintf (stderr, "\t-t\t%s\n",       shelp[lang][ 9]);
    (void)fprintf (stderr, "\t-X\t%s\n",       shelp[lang][ 8]);
    (void)fprintf (stderr, "\t-m dir\t%s\n",   shelp[lang][ 4]);
    (void)fprintf (stderr, "\tmenu\t%s\n",     shelp[lang][ 5]);
    return (0);
    } /* help */

int main (int argc, char *argv[])
{
    int  i, j;
    char *mdir = NIL, *mdef; /* MENUDIR, MENUDEF */
    char m_name[80];

#ifdef SEQUENT
    setdtablesize (256);
#endif
    signal ( 2, SIG_IGN);
    signal ( 3, SIG_IGN);
    signal (15, SIG_DFL);

    if (argc == 2) {
	if (strcmp (argv[1], "-version") == 0) {
	    (void)fprintf (stderr, VERSION);
	    return (0);
	    }
	}

    j = 1;
    if ((args[1] = getenv ("CSHMEN")) && *args[1]) {
	*args = *argv;
	i = 2;
	while (j < argc)
	    args[i++] = argv[j++];
	args[i] = NIL;
	argv = args;
	argc++;
	j = 2;
	}
    while ((i = getopt (argc, argv, "adeNm:nutXv?")) != EOF) {
	switch (i) {
	    case 'X': Xgui ^= 1;     break;	/* try (NOT) using xamen */
	    case 'e': lang = 1;      break;
	    case 'N': lang = 0;      break;
	    case 'd': dflg++;        break;
	    case 'v': vflg++;        break;
	    case 'a': aflg++;        break;
	    case 't': tflg++;        break;
	    case 'n': nflg = 0;      break;
	    case 'u': uflg++;        break;
	    case 'm': mdir = optarg; break;
	    case '?':
		if (argc == j + 1 && strcmp ("-?", argv[j]) == 0)
		    return (help (argv[0]));
	    default:
		return (usage (argv[0]));
	    }
	}
    if (argc - optind > 1)
	return (usage (argv[0]));

    if (Xgui && getenv ("DISPLAY"))
	execvp ("xamen", argv);

    if (mdir || (mdir = getenv ("MENUDIR"))) {
	if (chdir (mdir)) { 
	    perror (mdir);
	    return (-1);
	    }
	}
    if (argc - optind == 0) {
	if (!mdir && !(mdir = getenv ("HOME"))) {
	    printf ("cshmen: HOME ");
	    printf ("%s in environment\n", lang ?
		"variable not found" : "variabele niet gevonden");
	    return (-1);
	    }
	if (!(mdef = getenv ("MENUDEF")))
	    mdef = ".cshmen";
	(void)sprintf (m_name, "%s/%s", mdir, mdef);
	}
    else
	strcpy (m_name, argv[optind]);

    if ((menus[0] = get_men (m_name, 0)) == (Menu *)0)
	return (-1);
    init_men ();
    run_menu ();
    clear_all ();
    return (0);
    } /* main */
