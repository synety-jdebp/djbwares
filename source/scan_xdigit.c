/* Public domain. */

#include "scan.h"

unsigned int scan_xdigit(register char ch)
{
  if ((ch >= '0') && (ch <= '9'))
    return ch - '0';
  else if ((ch >= 'a') && (ch <= 'f'))
    return ch - 'a' + 10;
  else if ((ch >= 'A') && (ch <= 'F'))
    return ch - 'A' + 10;
  else
    return -1;
}
