#include <sys/ioctl.h>
#include <termios.h>

int main()
{
  ioctl(1,TIOCNXCL,(char *) 0);
}
