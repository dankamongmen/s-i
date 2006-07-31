/*
 * loadkeys.y
 *
 * Taken from kbd-1.08 and hacked by amck, 2003-01-17
 * Stripped down version for use in kbd-chooser.
 */

%token EOL NUMBER LITERAL CHARSET KEYMAPS KEYCODE EQUALS
%token PLAIN SHIFT CONTROL ALT ALTGR SHIFTL SHIFTR CTRLL CTRLR
%token COMMA DASH STRING STRLITERAL COMPOSE TO CCHAR ERROR PLUS
%token UNUMBER ALT_IS_META STRINGS AS USUAL ON FOR

%{
#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "paths.h"
#include "getfd.h"
#include "findfile.h"
#include "modifiers.h"
#include "nls.h"
#include "keyboard.h"

#ifndef KT_LETTER
#define KT_LETTER KT_LATIN
#endif

/* Unfortunately NR_KEYS, defined in kernel-headers, changes
 * with kernel. Its
 *  128 on 2.4.*
 *  512 on 2.6.*
 *  256 on 2.6.1 (+?)
 * We redefine it here, for safety
 */
#ifdef NR_KEYS
#undef NR_KEYS
#endif
#define NR_KEYS 256

/* What keymaps are we defining? */
char defining[MAX_NR_KEYMAPS];
char keymaps_line_seen = 0;
int max_keymap = 0;		/* from here on, defining[] is false */
int alt_is_meta = 0;

/* the kernel structures we want to set or print */
u_short *key_map[MAX_NR_KEYMAPS];
char *func_table[MAX_NR_FUNC];
struct kbdiacr accent_table[MAX_DIACR];
unsigned int accent_table_size = 0;

char key_is_constant[NR_KEYS];
char *keymap_was_set[MAX_NR_KEYMAPS];
char func_buf[4096];		/* should be allocated dynamically */
char *fp = func_buf;

#define U(x) ((x) ^ 0xf000)

#undef ECHO

static void addmap(int map, int explicit);
static void addkey(int index, int table, int keycode);
static void addfunc(struct kbsentry kbs_buf);
static void killkey(int index, int table);
static void compose(int diacr, int base, int res);
static void do_constant(void);
static void do_constant_key (int, u_short);
static void loadkeys(int fd);
static void strings_as_usual(void);
static void compose_as_usual(const char *charset);
extern int set_charset(const char *charset);
extern int getfd(void);

int key_buf[MAX_NR_KEYMAPS];
int mod;
int private_error_ct = 0;

extern int rvalct;
extern struct kbsentry kbs_buf;

#include "ksyms.h"

int yyerror(const char *s);
int yylex(void);
%}

%%
keytable	:
		| keytable line
		;
line		: EOL
		| charsetline
		| altismetaline
		| usualstringsline
		| usualcomposeline
		| keymapline
		| fullline
		| singleline
		| strline
                | compline
		;
charsetline	: CHARSET STRLITERAL EOL
			{
			    set_charset((const char *) kbs_buf.kb_string);
			}
		;
altismetaline	: ALT_IS_META EOL
			{
			    alt_is_meta = 1;
			}
		;
usualstringsline: STRINGS AS USUAL EOL
			{
			    strings_as_usual();
			}
		;
usualcomposeline: COMPOSE AS USUAL FOR STRLITERAL EOL
			{
			    compose_as_usual((const char *) kbs_buf.kb_string);
			}
		  | COMPOSE AS USUAL EOL
			{
			    compose_as_usual(0);
			}
		;
keymapline	: KEYMAPS range EOL
			{
			    keymaps_line_seen = 1;
			}
		;
range		: range COMMA range0
		| range0
		;
range0		: NUMBER DASH NUMBER
			{
			    int i;
			    for (i = $1; i<= $3; i++)
			      addmap(i,1);
			}
		| NUMBER
			{
			    addmap($1,1);
			}
		;
strline		: STRING LITERAL EQUALS STRLITERAL EOL
			{
			    if (KTYP($2) != KT_FN)
				lkfatal1(_("'%s' is not a function key symbol"),
					syms[KTYP($2)].table[KVAL($2)]);
			    kbs_buf.kb_func = KVAL($2);
			    addfunc(kbs_buf);
			}
		;
compline        : COMPOSE CCHAR CCHAR TO CCHAR EOL
                        {
			    compose($2, $3, $5);
			}
		 | COMPOSE CCHAR CCHAR TO rvalue EOL
			{
			    compose($2, $3, $5);
			}
                ;
singleline	:	{ mod = 0; }
		  modifiers KEYCODE NUMBER EQUALS rvalue EOL
			{
			    addkey($4, mod, $6);
			}
		| PLAIN KEYCODE NUMBER EQUALS rvalue EOL
			{
			    addkey($4, 0, $6);
			}
		;
modifiers	: modifiers modifier
		| modifier
		;
modifier	: SHIFT		{ mod |= M_SHIFT;	}
		| CONTROL	{ mod |= M_CTRL;	}
		| ALT		{ mod |= M_ALT;		}
		| ALTGR		{ mod |= M_ALTGR;	}
		| SHIFTL	{ mod |= M_SHIFTL;	}
		| SHIFTR	{ mod |= M_SHIFTR;	}
		| CTRLL		{ mod |= M_CTRLL;	}
		| CTRLR		{ mod |= M_CTRLR;	}
		;
fullline	: KEYCODE NUMBER EQUALS rvalue0 EOL
	{
	    int i, j;

	    if (rvalct == 1) {
	      /* Some files do not have a keymaps line, and
		 we have to wait until all input has been read
		 before we know which maps to fill. */
	      key_is_constant[$2] = 1;

	      /* On the other hand, we now have include files,
		 and it should be possible to override lines
		 from an include file. So, kill old defs. */
	      for (j = 0; j < max_keymap; j++)
		if (defining[j])
		  killkey($2, j);
	    }
	    if (keymaps_line_seen) {
		i = 0;
		for (j = 0; j < max_keymap; j++)
		  if (defining[j]) {
		      if (rvalct != 1 || i == 0)
			addkey($2, j, (i < rvalct) ? key_buf[i] : K_HOLE);
		      i++;
		  }
		if (i < rvalct)
		    lkfatal0(_("too many (%d) entries on one line"), rvalct);
	    } else
	      for (i = 0; i < rvalct; i++)
		addkey($2, i, key_buf[i]);
	}
		;

rvalue0		: 
		| rvalue1 rvalue0
		;
rvalue1		: rvalue
			{
			    if (rvalct >= MAX_NR_KEYMAPS)
				lkfatal(_("too many keydefinitions on one line"));
			    key_buf[rvalct++] = $1;
			}
		;
rvalue		: NUMBER
			{$$=add_number($1);}
		| LITERAL
			{$$=add_number($1);}
		| UNUMBER
			{$$=add_number($1);}
                | PLUS NUMBER
                        {$$=add_capslock($2);}
		| PLUS UNUMBER
			{$$=add_capslock($2);}
                | PLUS LITERAL
                        {$$=add_capslock($2);}
		;
%%			

#include "analyze.c"

char *keymap_name;
int nocompose = 0;

void loadkeys_wrapper (char *map)
{
	int fd;

	fd = getfd();
	keymap_name = map;

	yywrap ();
	if (yyparse() || private_error_ct) {
		di_error ("kbd-chooser: Syntax error in keymap\n");
		exit (1);
	}
	do_constant();
	loadkeys (fd);
	exit (0);
}

extern char pathname[];
char *filename;
int line_nr = 1;

int
yyerror(const char *s) {
	di_error("kbd-chooser: %s:%d: %s\n", filename, line_nr, s);
	private_error_ct++;
	return(0);
}

/* Include file handling - unfortunately flex-specific. */
#define MAX_INCLUDE_DEPTH 20
struct infile {
	int linenr;
	char *filename;
	YY_BUFFER_STATE bs;
} infile_stack[MAX_INCLUDE_DEPTH];
int infile_stack_ptr = 0;

void
lk_push(void) {
	if (infile_stack_ptr >= MAX_INCLUDE_DEPTH)
		lkfatal(_("includes nested too deeply"));

	/* preserve current state */
	infile_stack[infile_stack_ptr].filename = filename;
	infile_stack[infile_stack_ptr].linenr = line_nr;
	infile_stack[infile_stack_ptr++].bs =
		YY_CURRENT_BUFFER;
}

int
lk_pop(void) {
	if (--infile_stack_ptr >= 0) {
		filename = infile_stack[infile_stack_ptr].filename;
		line_nr = infile_stack[infile_stack_ptr].linenr;
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(infile_stack[infile_stack_ptr].bs);
		return 0;
	}
	return 1;
}

/*
 * Where shall we look for an include file?
 * Current strategy (undocumented, may change):
 *
 * 1. Look for a user-specified LOADKEYS_INCLUDE_PATH
 * 2. Try . and ../include and ../../include
 * 3. Try D and D/../include and D/../../include
 *    where D is the directory from where we are loading the current file.
 * 4. Try KD/include and KD/#/include where KD = DATADIR/KEYMAPDIR.
 *
 * Expected layout:
 * KD has subdirectories amiga, atari, i386, mac, sun, include
 * KD/include contains architecture-independent stuff
 * like strings and iso-8859-x compose tables.
 * KD/i386 has subdirectories qwerty, ... and include;
 * this latter include dir contains stuff with keycode=...
 *
 * (Of course, if the present setup turns out to be reasonable,
 * then later also the other architectures will grow and get
 * subdirectories, and the hard-coded i386 below will go again.)
 *
 * People that dislike a dozen lookups for loadkeys
 * can easily do "loadkeys file_with_includes; dumpkeys > my_keymap"
 * and afterwards use only "loadkeys /fullpath/mykeymap", where no
 * lookups are required.
 */
char *include_dirpath0[] = { "", 0 };
char *include_dirpath1[] = { "", "../include/", "../../include/", 0 };
char *include_dirpath2[] = { 0, 0, 0, 0 };
char *include_dirpath3[] = { DATADIR "/" KEYMAPDIR "/include/",
			     DATADIR "/" KEYMAPDIR "/i386/include/",
			     DATADIR "/" KEYMAPDIR "/mac/include/", 0 };
char *include_suffixes[] = { "", ".inc", 0 };

FILE *find_incl_file_near_fn(char *s, char *fn) {
	FILE *f = NULL;
	char *t, *te, *t1, *t2;
	int len;

	if (!fn)
		return NULL;

	t = xstrdup(fn);
	te = rindex(t, '/');
	if (te) {
		te[1] = 0;
		include_dirpath2[0] = t;
		len = strlen(t);
		include_dirpath2[1] = t1 = xmalloc(len + 12);
		include_dirpath2[2] = t2 = xmalloc(len + 15);
		strcpy(t1, t);
		strcat(t1, "../include/");
		strcpy(t2, t);
		strcat(t2, "../../include/");
		f = findfile(s, include_dirpath2, include_suffixes);
		if (f)
			return f;
	}
	return f;
}

FILE *find_standard_incl_file(char *s) {
	FILE *f;

	f = findfile(s, include_dirpath1, include_suffixes);
	if (!f)
		f = find_incl_file_near_fn(s, filename);

	/* If filename is a symlink, also look near its target. */
	if (!f) {
		char buf[1024], path[1024], *p;
		int n;

		n = readlink(filename, buf, sizeof(buf));
		if (n > 0 && n < sizeof(buf)) {
		     buf[n] = 0;
		     if (buf[0] == '/')
			  f = find_incl_file_near_fn(s, buf);
		     else if (strlen(filename) + n < sizeof(path)) {
			  strcpy(path, filename);
			  path[sizeof(path)-1] = 0;
			  p = rindex(path, '/');
			  if (p)
			       p[1] = 0;
			  strcat(path, buf);
			  f = find_incl_file_near_fn(s, path);
		     }
		}
	}

	if (!f)
	     f = findfile(s, include_dirpath3, include_suffixes);
	return f;
}

FILE *find_incl_file(char *s) {
	FILE *f;
	char *ev;
	if (!s || !*s)
		return NULL;
	if (*s == '/')		/* no path required */
		return (findfile(s, include_dirpath0, include_suffixes));

	if((ev = getenv("LOADKEYS_INCLUDE_PATH")) != NULL) {
		/* try user-specified path */
		char *user_dir[2] = { 0, 0 };
		while(ev) {
			char *t = index(ev, ':');
			char sv = '\0';
			if (t) {
				sv = *t;
				*t = 0;
			}
			user_dir[0] = ev;
			if (*ev)
				f = findfile(s, user_dir, include_suffixes);
			else	/* empty string denotes system path */
				f = find_standard_incl_file(s);
			if (f)
				return f;
			if (t)
				*t++ = sv;
			ev = t;
		}
		return NULL;
	}
	return find_standard_incl_file(s);
}

void
open_include(char *s) {

	lk_push();

	yyin = find_incl_file(s);
	if (!yyin)
		lkfatal1(_("cannot open include file %s"), s);
	filename = xstrdup(pathname);
	line_nr = 1;
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
}

/* String file handling - flex-specific. */
int in_string = 0;

void
lk_scan_string(char *s) {
	lk_push();
	in_string = 1;
	yy_scan_string(s);
}

void
lk_end_string(void) {
	lk_pop();
	in_string = 0;
}

char *dirpath[] = { "", DATADIR "/" KEYMAPDIR "/**",  "/", 0 };
char *suffixes[] = { "", ".kmap", 0 };

#undef yywrap
int
yywrap(void) {
	FILE *f;
	static int first_file = 1, done = 0; /* ugly kludge flag */

	if (in_string) {
		lk_end_string();
		return 0;
	}

	if (infile_stack_ptr > 0) {
		lk_pop();
		return 0;
	}

	line_nr = 1;

	if (done)
		return  1;
	if ((f = findfile(keymap_name, dirpath, suffixes)) == NULL) {
		di_error ("kbd-chooser: cannot open file %s\n", keymap_name);
		exit(1);
	}
	/*
		Can't use yyrestart if this is called before entering yyparse()
		I think assigning directly to yyin isn't necessarily safe in
		other situations, hence the flag.
	*/
	filename = xstrdup(pathname);
	if (first_file) {
		yyin = f;
		first_file = 0;
		done = 1;
	} else
		yyrestart (f);
	return 0;
}

static void
addmap(int i, int explicit) {
	if (i < 0 || i >= MAX_NR_KEYMAPS)
	    lkfatal0(_("addmap called with bad index %d"), i);

	if (!defining[i]) {
	    if (keymaps_line_seen && !explicit)
		lkfatal0(_("adding map %d violates explicit keymaps line"), i);

	    defining[i] = 1;
	    if (max_keymap <= i)
	      max_keymap = i+1;
	}
}

/* unset a key */
static void
killkey(int index, int table) {
	/* roughly: addkey(index, table, K_HOLE); */

        if (index < 0 || index >= NR_KEYS)
	        lkfatal0(_("killkey called with bad index %d"), index);
        if (table < 0 || table >= MAX_NR_KEYMAPS)
	        lkfatal0(_("killkey called with bad table %d"), table);
	if (key_map[table])
		(key_map[table])[index] = K_HOLE;
	if (keymap_was_set[table])
		(keymap_was_set[table])[index] = 0;
}

static void
addkey(int index, int table, int keycode) {
	int i;

	if (keycode == -1)
		return;
        if (index < 0 || index >= NR_KEYS)
	        lkfatal0(_("addkey called with bad index %d"), index);
        if (table < 0 || table >= MAX_NR_KEYMAPS)
	        lkfatal0(_("addkey called with bad table %d"), table);

	if (!defining[table])
		addmap(table, 0);
	if (!key_map[table]) {
	        key_map[table] = (u_short *)xmalloc(NR_KEYS * sizeof(u_short));
		for (i = 0; i < NR_KEYS; i++)
		  (key_map[table])[i] = K_HOLE;
	}
	if (!keymap_was_set[table]) {
	        keymap_was_set[table] = (char *) xmalloc(NR_KEYS);
		for (i = 0; i < NR_KEYS; i++)
		  (keymap_was_set[table])[i] = 0;
	}

	if (alt_is_meta && keycode == K_HOLE && (keymap_was_set[table])[index])
		return;

	(key_map[table])[index] = keycode;
	(keymap_was_set[table])[index] = 1;

	if (alt_is_meta) {
	     int alttable = table | M_ALT;
	     int type = KTYP(keycode);
	     int val = KVAL(keycode);
	     if (alttable != table && defining[alttable] &&
		 (!keymap_was_set[alttable] ||
		  !(keymap_was_set[alttable])[index]) &&
		 (type == KT_LATIN || type == KT_LETTER) && val < 128)
		  addkey(index, alttable, K(KT_META, val));
	}
}

static void
addfunc(struct kbsentry kbs) {
        int sh, i;
	char *p, *q, *r;

	if ((q = func_table[kbs.kb_func])) { /* throw out old previous def */
	        sh = strlen(q) + 1;
		p = q + sh;
		while (p < fp)
		        *q++ = *p++;
		fp -= sh;

		for (i = kbs.kb_func + 1; i < MAX_NR_FUNC; i++)
		     if (func_table[i])
			  func_table[i] -= sh;
	}

	p = func_buf;                        /* find place for new def */
	for (i = 0; i < kbs.kb_func; i++)
	        if (func_table[i]) {
		        p = func_table[i];
			while(*p++);
		}
	func_table[kbs.kb_func] = p;
        sh = strlen((const char *) kbs.kb_string) + 1;
	if (fp + sh > func_buf + sizeof(func_buf)) {
	       di_error  ("%s: addfunc: func_buf overflow\n", PROGNAME);
		exit(1);
	}
	q = fp;
	fp += sh;
	r = fp;
	while (q > p)
	        *--r = *--q;
	strcpy(p, (const char *) kbs.kb_string);
	for (i = kbs.kb_func + 1; i < MAX_NR_FUNC; i++)
	        if (func_table[i])
		        func_table[i] += sh;
}

static void
compose(int diacr, int base, int res) {
        struct kbdiacr *p;
        if (accent_table_size == MAX_DIACR) {
	        di_error ( " compose table overflow\n");
		exit(1);
	}
	p = &accent_table[accent_table_size++];
	p->diacr = diacr;
	p->base = base;
	p->result = res;
}

static int
defkeys(int fd) {
	struct kbentry ke;
	int ct = 0;
	int i,j,fail, warnings=0;

	for(i=0; i<MAX_NR_KEYMAPS; i++) {
	    if (key_map[i]) {
		for(j=0; j<NR_KEYS; j++) {
		    if ((keymap_was_set[i])[j]) {
			ke.kb_index = j;
			ke.kb_table = i;
			ke.kb_value = (key_map[i])[j];

			fail = ioctl(fd, KDSKBENT, (unsigned long)&ke);
			if (fail) {
			    if (errno == EPERM) {
			    	di_error ("kbd-chooser: Keymap %d: Permission denied\n", i);
				j = NR_KEYS;
				continue;
			    }
			    if (errno == EINVAL) {
			    	if (warnings++ == 0)
				    	di_warning ("kbd-chooser: Ignoring invalid keys\n");
				continue;
			    }
			} else
			  ct++;
			if (fail)
				di_error ("kbd-chooser: failed to bind key %d to value %d\n",
				  	j, (key_map[i])[j]);
		    }
		}
	    } else if (keymaps_line_seen && !defining[i]) {
		/* deallocate keymap */
		ke.kb_index = 0;
		ke.kb_table = i;
		ke.kb_value = K_NOSUCHMAP;

		if(ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
		    if (errno != EINVAL) {
				di_error("%s: could not deallocate keymap %d (%s)\n",
				PROGNAME, i, strerror(errno));
			exit(1);
		    }
		    /* probably an old kernel */
		    /* clear keymap by hand */
		    for (j = 0; j < NR_KEYS; j++) {
			ke.kb_index = j;
			ke.kb_table = i;
			ke.kb_value = K_HOLE;
			if(ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
			    if (errno == EINVAL && i >= 16)
			      break; /* old kernel */
				    di_error("%s: cannot deallocate or clear keymap (%s)\n",
				    PROGNAME, strerror(errno));
			    exit(1);
			}
		    }
		}
	    }
	}

	return ct;
}

static char *
ostr(const char *s) {
	int lth = strlen(s);
	char *ns0 = xmalloc(4*lth + 1);
	char *ns = ns0;

	while(*s) {
	  switch(*s) {
	  case '\n':
	    *ns++ = '\\';
	    *ns++ = 'n';
	    break;
	  case '\033':
	    *ns++ = '\\';
	    *ns++ = '0';
	    *ns++ = '3';
	    *ns++ = '3';
	    break;
	  default:
	    *ns++ = *s;
	  }
	  s++;
	}
	*ns = 0;
	return ns0;
}

static int
deffuncs(int fd){
        int i, ct = 0;
	char *p;

        for (i = 0; i < MAX_NR_FUNC; i++) {
	    kbs_buf.kb_func = i;
	    if ((p = func_table[i])) {
		strcpy((char *) kbs_buf.kb_string, p);
		if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs_buf))
		  di_error ("failed to bind string '%s' to function %s\n",
			  ostr((const char *) kbs_buf.kb_string), syms[KT_FN].table[kbs_buf.kb_func]);
		else
		  ct++;
	    } 
	  }
	return ct;
}

static int
defdiacs(int fd){
        struct kbdiacrs kd;
	int i;

	kd.kb_cnt = accent_table_size;
	if (kd.kb_cnt > MAX_DIACR) {
	    kd.kb_cnt = MAX_DIACR;
	    di_error("kbd-chooser: too many compose definitions\n");
	}
	for (i = 0; i < kd.kb_cnt; i++)
	    kd.kbdiacr[i] = accent_table[i];

	if(ioctl(fd, KDSKBDIACR, (unsigned long) &kd)) {
	    di_error("kbd-chooser: KDSKBDIACR (%s)", strerror(errno));
	    exit(1);
	}
	return kd.kb_cnt;
}

void
do_constant_key (int i, u_short key) {
	int typ, val, j;

	typ = KTYP(key);
	val = KVAL(key);
	if ((typ == KT_LATIN || typ == KT_LETTER) &&
	    ((val >= 'a' && val <= 'z') ||
	     (val >= 'A' && val <= 'Z'))) {
		u_short defs[16];
		defs[0] = K(KT_LETTER, val);
		defs[1] = K(KT_LETTER, val ^ 32);
		defs[2] = defs[0];
		defs[3] = defs[1];
		for(j=4; j<8; j++)
			defs[j] = K(KT_LATIN, val & ~96);
		for(j=8; j<16; j++)
			defs[j] = K(KT_META, KVAL(defs[j-8]));
		for(j=0; j<max_keymap; j++) {
			if (!defining[j])
				continue;
			if (j > 0 &&
			    keymap_was_set[j] && (keymap_was_set[j])[i])
				continue;
			addkey(i, j, defs[j%16]);
		}
	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j=1; j<max_keymap; j++)
			if(defining[j] &&
			    (!(keymap_was_set[j]) || !(keymap_was_set[j])[i]))
				addkey(i, j, key);
	}
}

static void
do_constant (void) {
	int i, r0 = 0;

	if (keymaps_line_seen)
		while (r0 < max_keymap && !defining[r0])
			r0++;

	for (i=0; i<NR_KEYS; i++) {
		if (key_is_constant[i]) {
			u_short key;
			if (!key_map[r0])
				lkfatal(_("impossible error in do_constant"));
			key = (key_map[r0])[i];
			do_constant_key (i, key);
		}
	}
}

static void
loadkeys (int fd) {
        int keyct, funcct, diacct;

	keyct = defkeys(fd);
	funcct = deffuncs(fd);
	if (accent_table_size > 0 || nocompose)
		diacct = defdiacs(fd);
}

static void strings_as_usual(void) {
	/*
	 * 26 strings, mostly inspired by the VT100 family
	 */
	char *stringvalues[30] = {
		/* F1 .. F20 */
		"\033[[A", "\033[[B", "\033[[C", "\033[[D", "\033[[E",
		"\033[17~", "\033[18~", "\033[19~", "\033[20~", "\033[21~",
		"\033[23~", "\033[24~", "\033[25~", "\033[26~",
		"\033[28~", "\033[29~",
		"\033[31~", "\033[32~", "\033[33~", "\033[34~",
		/* Find,    Insert,    Remove,    Select,    Prior */
		"\033[1~", "\033[2~", "\033[3~", "\033[4~", "\033[5~",
		/* Next,    Macro,  Help, Do,  Pause */
		"\033[6~",    0,      0,   0,    0
	};
	int i;
	for (i=0; i<30; i++) if(stringvalues[i]) {
		struct kbsentry ke;
		ke.kb_func = i;
		strncpy((char *) ke.kb_string, stringvalues[i], sizeof(ke.kb_string));
		ke.kb_string[sizeof(ke.kb_string)-1] = 0;
		addfunc(ke);
	}
}

static void
compose_as_usual(const char *charset) {
	if (charset && strcmp(charset, "iso-8859-1")) {
		di_error("kbd-chooser: loadkeys: don't know how to compose for %s\n",
			charset);
		exit(1);
	} else {
		struct ccc {
			char c1, c2, c3;
		} def_latin1_composes[68] = {
			{ '`', 'A', 0300 }, { '`', 'a', 0340 },
			{ '\'', 'A', 0301 }, { '\'', 'a', 0341 },
			{ '^', 'A', 0302 }, { '^', 'a', 0342 },
			{ '~', 'A', 0303 }, { '~', 'a', 0343 },
			{ '"', 'A', 0304 }, { '"', 'a', 0344 },
			{ 'O', 'A', 0305 }, { 'o', 'a', 0345 },
			{ '0', 'A', 0305 }, { '0', 'a', 0345 },
			{ 'A', 'A', 0305 }, { 'a', 'a', 0345 },
			{ 'A', 'E', 0306 }, { 'a', 'e', 0346 },
			{ ',', 'C', 0307 }, { ',', 'c', 0347 },
			{ '`', 'E', 0310 }, { '`', 'e', 0350 },
			{ '\'', 'E', 0311 }, { '\'', 'e', 0351 },
			{ '^', 'E', 0312 }, { '^', 'e', 0352 },
			{ '"', 'E', 0313 }, { '"', 'e', 0353 },
			{ '`', 'I', 0314 }, { '`', 'i', 0354 },
			{ '\'', 'I', 0315 }, { '\'', 'i', 0355 },
			{ '^', 'I', 0316 }, { '^', 'i', 0356 },
			{ '"', 'I', 0317 }, { '"', 'i', 0357 },
			{ '-', 'D', 0320 }, { '-', 'd', 0360 },
			{ '~', 'N', 0321 }, { '~', 'n', 0361 },
			{ '`', 'O', 0322 }, { '`', 'o', 0362 },
			{ '\'', 'O', 0323 }, { '\'', 'o', 0363 },
			{ '^', 'O', 0324 }, { '^', 'o', 0364 },
			{ '~', 'O', 0325 }, { '~', 'o', 0365 },
			{ '"', 'O', 0326 }, { '"', 'o', 0366 },
			{ '/', 'O', 0330 }, { '/', 'o', 0370 },
			{ '`', 'U', 0331 }, { '`', 'u', 0371 },
			{ '\'', 'U', 0332 }, { '\'', 'u', 0372 },
			{ '^', 'U', 0333 }, { '^', 'u', 0373 },
			{ '"', 'U', 0334 }, { '"', 'u', 0374 },
			{ '\'', 'Y', 0335 }, { '\'', 'y', 0375 },
			{ 'T', 'H', 0336 }, { 't', 'h', 0376 },
			{ 's', 's', 0337 }, { '"', 'y', 0377 },
			{ 's', 'z', 0337 }, { 'i', 'j', 0377 }
		};
		int i;
		for(i=0; i<68; i++) {
			struct ccc p = def_latin1_composes[i];
			compose(p.c1, p.c2, p.c3);
		}
	}
}

