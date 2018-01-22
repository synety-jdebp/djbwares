#include "strerr.h"
#include "stralloc.h"
#include "getln.h"
#include "buffer.h"
#include "exit.h"
#include "fmt.h"
#include "byte.h"
#include "scan.h"
#include "open.h"
#include "cdb_make.h"
#include <unistd.h>

#define FATAL "tcprules: fatal: "

unsigned long linenum = 0;
char *fntemp;
char *fn;

stralloc line = stralloc_static_0;
int match = 1;

stralloc address = stralloc_static_0;
stralloc data = stralloc_static_0;
stralloc key = stralloc_static_0;

struct cdb_make c;

void nomem(void)
{
  strerr_die2x(111,FATAL,"out of memory");
}
void usage(void)
{
  strerr_die1x(100,"tcprules: usage: tcprules rules.cdb rules.tmp");
}
void die_bad(void)
{
  if (!stralloc_0(&line)) nomem();
  strerr_die3x(100,FATAL,"unable to parse this line: ",line.s);
}
void die_write(void)
{
  strerr_die4sys(111,FATAL,"unable to write to ",fntemp,": ");
}

char strnum[FMT_ULONG];
stralloc sanum = stralloc_static_0;

void getnum(char *buf,int len,unsigned long *u)
{
  if (!stralloc_copyb(&sanum,buf,len)) nomem();
  if (!stralloc_0(&sanum)) nomem();
  if (sanum.s[scan_ulong(sanum.s,u)]) die_bad();
}

void doaddressdata(void)
{
  int i;
  int left;
  int right;
  unsigned long bot;
  unsigned long top;

  if (byte_chr(address.s,address.len,'=') == address.len)
    if (byte_chr(address.s,address.len,'@') == address.len) {
      i = byte_chr(address.s,address.len,'-');
      if (i < address.len) {
        left = byte_rchr(address.s,i,'.');
        if (left == i) left = 0; else ++left;
  
        ++i;
        right = i + byte_chr(address.s + i,address.len - i,'.');
  
        getnum(address.s + left,i - 1 - left,&bot);
        getnum(address.s + i,right - i,&top);
        if (top > 255) top = 255;
  
        while (bot <= top) {
	  if (!stralloc_copyb(&key,address.s,left)) nomem();
	  if (!stralloc_catb(&key,strnum,fmt_ulong(strnum,bot))) nomem();
	  if (!stralloc_catb(&key,address.s + right,address.len - right)) nomem();
          if (cdb_make_add(&c,key.s,key.len,data.s,data.len) == -1) die_write();
	  ++bot;
        }
  
        return;
      }
    }

  if (cdb_make_add(&c,address.s,address.len,data.s,data.len) == -1) die_write();
}

int main(int argc,char **argv)
{
  int colon;
  char *x;
  int len;
  int fd;
  int i;
  char ch;
  (void)argc;	/* Silence a compiler warning. */

  fn = argv[1];
  if (!fn) usage();
  fntemp = argv[2];
  if (!fntemp) usage();

  fd = open_trunc(fntemp);
  if (fd == -1)
    strerr_die4sys(111,FATAL,"unable to create ",fntemp,": ");
  if (cdb_make_start(&c,fd) == -1) die_write();

  while (match) {
    if (getln(buffer_0,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,"unable to read input: ");

    x = line.s; len = line.len;

    if (!len) break;
    if (x[0] == '#') continue;
    if (x[0] == '\n') continue;

    while (len) {
      ch = x[len - 1];
      if (ch != '\n') if (ch != ' ') if (ch != '\t') break;
      --len;
    }
    line.len = len; /* for die_bad() */

    colon = byte_chr(x,len,':');
    if (colon == len) continue;

    if (!stralloc_copyb(&address,x,colon)) nomem();
    if (!stralloc_copys(&data,"")) nomem();

    x += colon + 1; len -= colon + 1;

    if ((len >= 4) && byte_equal(x,4,"deny")) {
      if (!stralloc_catb(&data,"D",2)) nomem();
      x += 4; len -= 4;
    }
    else if ((len >= 5) && byte_equal(x,5,"allow")) {
      x += 5; len -= 5;
    }
    else
      die_bad();

    while (len)
      switch(*x) {
        case ',':
          i = byte_chr(x,len,'=');
          if (i == len) die_bad();
          if (!stralloc_catb(&data,"+",1)) nomem();
          if (!stralloc_catb(&data,x + 1,i)) nomem();
          x += i + 1; len -= i + 1;
          if (!len) die_bad();
          ch = *x;
          x += 1; len -= 1;
          i = byte_chr(x,len,ch);
          if (i == len) die_bad();
          if (!stralloc_catb(&data,x,i)) nomem();
          if (!stralloc_0(&data)) nomem();
          x += i + 1; len -= i + 1;
          break;
        default:
          die_bad();
      }

    doaddressdata();
  }

  if (cdb_make_finish(&c) == -1) die_write();
  if (fsync(fd) == -1) die_write();
  if (close(fd) == -1) die_write(); /* NFS stupidity */
  if (rename(fntemp,fn))
    strerr_die6sys(111,FATAL,"unable to move ",fntemp," to ",fn,": ");

  _exit(0);
}
