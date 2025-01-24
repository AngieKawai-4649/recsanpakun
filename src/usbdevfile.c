/* fsusb2i   (c) 2015 trinity19683
  USB device file (Linux OSes)
  usbdevfile.c
  2015-12-28
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/file.h>
#include <linux/usb/ch9.h>
#include <dirent.h>
#include <stdbool.h>

#ifdef DB_CARDREADER
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <PCSC/winscard.h>
#endif


#include "usbdevfile.h"
#include "message.h"

#define PATH_SEPARATOR  '/'

const char* BASE_DIR_UDEV = "/dev/bus/usb";


#ifdef DB_CARDREADER
static bool check_usbdevId(int vendor_id, int product_id, bool *flg)
#else
static bool check_usbdevId(int vendor_id, int product_id)
#endif
{
	bool rtn = false;
	if(0x0511 == vendor_id) {
		switch( product_id ) {
		//case 0x0029:
		//case 0x003b:
		case 0x0045:
			rtn = true;
#ifdef DB_CARDREADER
		// 内蔵カードリーダーの時、自デバイスのカードリーダーかチェックする
			char mszReaders[128];
			unsigned long pcchReaders;
			long hContext = 0;
			int rtn = SCardListReaders(hContext, NULL, mszReaders, &pcchReaders);
printf("check_usbdevId SCardListReaders return %d\n", rtn);
			if(rtn<0){
				return false;
			}
			char *p = strchr(mszReaders, ':');
			p++;
			*(p+4) = '\0';
			unsigned long vnd = strtoul(p, NULL, 16);
			p = strchr(p+5, ':');
			p++;
			*(p+4) = '\0';
			unsigned long pid = strtoul(p, NULL, 16);
			*flg = (vnd == vendor_id && pid == product_id);
#endif
			break;
		}
	}
	return rtn;
}

static int usb_getdesc(const char *devfile, struct usb_device_descriptor* desc)
{
	int f;

	f = open(devfile, O_RDONLY);
	if(-1 == f) {
		warn_msg(errno,"can't open usbdevfile to read '%s'", devfile);
		return errno;
	}
	memset(desc, 0, sizeof(struct usb_device_descriptor));
	ssize_t rlen = read(f, desc, sizeof(struct usb_device_descriptor));
	if(-1 == rlen) {
		warn_msg(errno,"can't read usbdevfile '%s'", devfile);
		return errno;
	}
	if(close(f) == -1) {
		warn_msg(errno,"can't close usbdevfile '%s'", devfile);
		return errno;
	}

	return 0;
}

static int path_strcat(char* const path, const int lpos, const char* const fname, const size_t maxlen)
{
	int len = strlen(fname);
	if(lpos + len >= maxlen -2) {
		//# buffer overflow
		return -1;
	}
	path[lpos] = PATH_SEPARATOR;
	memcpy(&path[lpos +1], fname, 1 + len);
	return lpos + 1 + len;
}

static HANDLE usb_open(const char *devfile)
{
	HANDLE fd;

	if((fd = open(devfile, O_RDWR)) == -1) {
		warn_msg(errno,"can't open usbdevfile to read/write '%s'", devfile);
		return -errno;
	}
	if(flock(fd, LOCK_EX | LOCK_NB) < 0) {
		warn_msg(errno,"share violation. usbdevfile '%s'", devfile);
		if(close(fd) == -1) {
			warn_msg(errno,"failed to close usbdevfile '%s'", devfile);
			//return -errno;
		}
		return -errno;
	}
	return fd;
}

#ifdef DB_CARDREADER
HANDLE usbdevfile_alloc(char** const pdevfile, int *vendor_id, int *product_id, bool *devflg)
#else
HANDLE usbdevfile_alloc(char** const pdevfile )
#endif
{
#define MAX_DEPTH_DIR  2
	DIR *dp[MAX_DEPTH_DIR];
	int depth, dirpath_len;
	char dirpath[PATH_MAX];

	if(NULL == pdevfile) {
		return -EINVAL;
	}
	if(NULL != *pdevfile) {
		struct usb_device_descriptor  desc;
		if(usb_getdesc(*pdevfile, &desc) == 0){
#ifdef DB_CARDREADER
			if(check_usbdevId(desc.idVendor,desc.idProduct, devflg)){
#else
			if(check_usbdevId(desc.idVendor,desc.idProduct)){
#endif
				return usb_open(*pdevfile);
			}
		}
		return -1;
	}

	strcpy(dirpath, BASE_DIR_UDEV);
	dirpath_len = strlen(BASE_DIR_UDEV);
	dp[0] = NULL;
	depth = 0;
	while(0 <= depth) {
		if(NULL == dp[depth]) {
			dp[depth] = opendir(dirpath);
		}
		if(NULL == dp[depth]) {
			warn_info(0,"failed to opendir '%s'", dirpath);
			depth--;
			continue;
		}
		struct dirent *dentry;
		while((dentry = readdir(dp[depth])) != NULL) {
			if(DT_CHR == dentry->d_type) {
				//# found USB devfile
				const int r = path_strcat(dirpath, dirpath_len, dentry->d_name, sizeof(dirpath));
				if(0 > r)
					goto endcheck_usbdevfile;    //# buffer overflow check

				struct usb_device_descriptor  desc;
				if(usb_getdesc(dirpath, &desc) != 0)
					goto endcheck_usbdevfile;
#ifdef DB_CARDREADER
				if(!check_usbdevId(desc.idVendor,desc.idProduct, devflg)){
#else
				if(!check_usbdevId(desc.idVendor,desc.idProduct)){
#endif
					goto endcheck_usbdevfile;
				}
				HANDLE fd = usb_open(dirpath);
				if(0 <= fd) {
					//# usb_open success
					*pdevfile = strdup(dirpath);
					return fd;
				}
endcheck_usbdevfile:
				dirpath[dirpath_len] = 0;

			}else if(MAX_DEPTH_DIR -1 > depth && DT_DIR == dentry->d_type) {
				if(strcmp(dentry->d_name,".") == 0 || strcmp(dentry->d_name,"..") == 0) {
					//# self or parent directory, skip
					continue;
				}
				const int len = path_strcat(dirpath, dirpath_len, dentry->d_name, sizeof(dirpath));
				if(0 <= len) {    //# buffer overflow check
					dirpath_len = len;
					dp[++depth] = NULL;
					break;
				}
			}
		}
		if(NULL == dentry) {
			//# end of directory stream
			closedir(dp[depth]);
			char* const ptr = strrchr(dirpath, PATH_SEPARATOR);
			if(NULL != ptr) {
				*ptr = 0;
				dirpath_len = (int)(ptr - dirpath);
			}
			depth--;
		}
	}
	return -1;
}

/*EOF*/
