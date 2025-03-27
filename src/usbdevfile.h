/* fsusb2i   (c) 2015 trinity19683
  USB device file
  usbdevfile.h
  2015-12-09
*/
#pragma once
#include "types_u.h"

#ifdef DB_CARDREADER
HANDLE usbdevfile_alloc(char** const pdevfile, int *vendor, int *product, bool *flg);
#else
HANDLE usbdevfile_alloc(char** const pdevfile);
#endif

/*EOF*/
