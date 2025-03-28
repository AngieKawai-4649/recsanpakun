/* SunPTV-USB   (c) 2016 trinity19683
  recfsusb2n program (Linux OSes)
  main.c
  2016-02-11
  License: GNU GPLv3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef DB_CARDREADER
#include <stdbool.h>
#endif

#include "usbdevfile.h"
#include "em287x.h"
#include "tc90522.h"
#include "mxl136.h"
#include "tda20142.h"
#include "tsthread.h"
#include "utils.h"

#include "verstr.h"
#include "message.h"


static void printTMCC(void* const pDemod, const unsigned tunerNum);
static unsigned char volatile  caughtSignal = 0;
static void sighandler(int arg)    { caughtSignal = 1; }


int main(int argc, char **argv)
{
	int ret = 0, i=0, j=0;
	struct Args args;
	struct timespec  time_begin, time_end;
	struct usb_endpoint_st  usbep;
	tsthread_ptr tsthr;
	em287x_state devst = NULL;
	void  *pDemod = NULL, *pTuner[2]={0};
	struct i2c_device_st* pI2C;

#ifdef DB_CARDREADER
	bool dev_flg;
	int vendor, product;
#endif

	msg("recsanpakun " PRG_VER " " PRG_BUILT " " PRG_TIMESTAMP ", (GPL) 2025 AngieKawai-4649\n");

	memset(&time_begin, '\0', sizeof(struct timespec));
	memset(&time_end,   '\0', sizeof(struct timespec));
	memset(&args,       '\0', sizeof(struct Args));
	
	ret = parseOption(argc, argv, &args);
	if(ret == CH_RETURN_NOTFOUND){
		char msgbuf[256];
		if(args.ts_id > 0){
			snprintf(msgbuf, 256, "TSID 0x%04x ", args.ts_id);
		}else{
			msgbuf[0] = '\0';
		}
		msg("ChannelKey %s %s NotFound\n", args.channel_info.channel_key, msgbuf );
		ret=1;
		goto end1;
	}

	unsigned  tunerNum;
	if(args.channel_info.tuning_space == SPI_UHF || args.channel_info.tuning_space == SPI_CATV){
		tunerNum = 0;
	}else if(args.channel_info.tuning_space == SPI_BS || args.channel_info.tuning_space == SPI_CS){
		tunerNum = 1;
	}else{
		goto end1;
	}


#ifdef DB_CARDREADER
	usbep.fd = usbdevfile_alloc( &args.devfile, &vendor, &product, &dev_flg);
#else
	usbep.fd = usbdevfile_alloc( &args.devfile);
#endif
	if(0 <= usbep.fd) {
		msg("# ISDB-T/S TV Tuner SANPAKUN: '%s', freq= %u kHz, duration= %u sec.\n", args.devfile, args.channel_info.freq, args.recsec);
		free(args.devfile);
	}else{
		warn_msg(0,"No device can be used.");
		ret = 40;	goto end1;
	}
	u_gettime(&time_begin);

#ifdef DB_CARDREADER
	if(em287x_create(&devst, &usbep, dev_flg) != 0) {
#else
	if(em287x_create(&devst, &usbep) != 0) {
#endif

		ret = 41;	goto end3;
	}
	//# demod
	if(tc90522_create(&pDemod) != 0) {
		ret = 44;	goto end4;
	}
	pI2C = tc90522_i2c_ptr(pDemod);
	pI2C->addr = 0x20;
	em287x_attach(devst, pI2C);
	if(tc90522_init(pDemod) != 0) {
		ret = 45;	goto end4;
	}
	//# GR tuner
	if(tunerNum==0) {
		//pTuner[0] = NULL;
		if(mxl136_create(&pTuner[0]) != 0) {
			ret = 46;	goto end5;
		}
		pI2C = mxl136_i2c_ptr(pTuner[0]);
		pI2C->addr = 0xc0;
		tc90522_attach(pDemod, 0, pI2C);
		if(mxl136_init(pTuner[0]) != 0) {
			ret = 47;	goto end5;
		}
	//# BS/CS tuner
	}else{
		//pTuner[1] = NULL;
		if(tda20142_create(&pTuner[1]) != 0) {
			ret = 48;	goto end5;
		}
		pI2C = tda20142_i2c_ptr(pTuner[1]);
		pI2C->addr = 0xa8;
		tc90522_attach(pDemod, 1, pI2C);
		if(tda20142_init(pTuner[1]) != 0) {
			ret = 49;	goto end5;
		}
	//#
	}


	u_gettime(&time_end);
	u_difftime(&time_begin, &time_end, NULL, &j);
	if(args.flags & 0x1) {  //# verbose
		u_difftime(&time_begin, &time_end, NULL, &j);
		msg("init: %ums, ", j);
	}

	for(i = 3; i > 0; i--) {  //# try 3 times
		u_gettime(&time_begin);
		if( tc90522_selectDevice(pDemod, tunerNum) ) {
			ret = 50;	goto end5;
		}
		if(tunerNum & 0x1) {
/***********************************
*			mxl136_sleep(pTuner[0]);
***********************************/
			if( tda20142_setFreq(pTuner[1], args.channel_info.freq) ) {
				warn_msg(ret,"failed to setFreq");
				ret = 51;	goto end5;
			}
		}else{
			mxl136_wakeup(pTuner[0]);
			if( mxl136_setFreq(pTuner[0], args.channel_info.freq) ) {
				warn_msg(ret,"failed to setFreq");
				ret = 51;	goto end5;
			}
		}
		usleep(30000);
		if( tc90522_resetDemod(pDemod, tunerNum) ) {
			ret = 52;	goto end5;
		}
		usleep(80000);
		if(tunerNum & 0x1) {
			if(args.ts_id > 0){
				msg("select stream physical tsid 0x%04x ", args.ts_id);
			}else{
				msg("select stream logical tsid 0x%04x ", args.channel_info.l_tsid);
			}
			for(j = 15; j > 0; j-- ) {
				ret = tc90522_selectStream(pDemod, tunerNum, (args.ts_id > 0) ? args.ts_id : args.channel_info.l_tsid );
				if(0 == ret)  break;
				else if(0 > ret) {
					ret = 53;	goto end5;
				}
				usleep(50000);
			}
		}
		u_gettime(&time_end);
		if(args.flags & 0x1) {  //# verbose
			u_difftime(&time_begin, &time_end, NULL, &j);
			msg("waitTuning: %ums, ", j);
		}
		u_gettime(&time_begin);
		for(j = 35; j > 0; j-- ) {
			unsigned statData[4];
			usleep(30000);
			if( tc90522_readStatistic(pDemod, tunerNum, statData) ) continue;
			ret = statData[0];
			if( ret & 0x10 )  goto channel_detected;
		}
		if(0 == j) {
			if(0 == ret) {
				warn_msg(0,"freq=%u kHz is empty.", args.channel_info.freq);
			}else{
				warn_msg(0,"freq=%u kHz is weak signal %02X.", args.channel_info.freq, ret);
			}
		}
	}
channel_detected:
	u_gettime(&time_end);
	if(!(ret & 0x10)) {
		warn_msg(0, "TS SYNC_LOCK failed");
		ret = 58;	goto end3;
	}
	if(args.flags & 0x1) {  //# verbose
		u_difftime(&time_begin, &time_end, NULL, &j);
		msg("waitStream: %ums, SYNC_LOCK=%c\n", j, (ret & 0x10)? 'Y':'N' );
	}
	printTMCC(pDemod, tunerNum);

	if(tsthread_create(&tsthr, &usbep) != 0) {
		ret = 60;	goto end6;
	}
	//# purge TS buffer data
	for(j = 0; j < 500; j++) {
		if(tsthread_read(tsthr, NULL) <= 0)  break;
	}
	//# OutputBuffer
	struct OutputBuffer *pOutputBuffer;
	pOutputBuffer = create_FileBufferedWriter( 768 * 1024, args.destfile );
	if(! pOutputBuffer ) {
		warn_msg(0,"failed to init FileBufferedWriter.");
		ret = 70;	goto end4;
	}else{
		struct OutputBuffer * const  pFileBufferedWriter = pOutputBuffer;
		j = (args.flags & 0x1000)? 1 : 0;
		pOutputBuffer = create_TSParser( 8192, pFileBufferedWriter, j, devst );
		if(! pOutputBuffer ) {
			warn_msg(0,"failed to init TS Parser.");
			OutputBuffer_release(pFileBufferedWriter);
			ret = 71;	goto end4;
		}
	}
	//# change signal handler
	setSignalHandler(1, sighandler);
	//# time of the begin of rec.
	u_gettime(&time_begin);

	//# main loop
	msg("# Start!\n");
	while(! caughtSignal) {
		if(0 < args.recsec) {
			u_gettime(&time_end);
			u_difftime(&time_begin, &time_end, &j, NULL);
			if(j >= args.recsec)  break;
		}

		void* pBuffer;
		int rlen;
		if((rlen = tsthread_read(tsthr, &pBuffer)) > 0) {
			if( 0 < rlen && (ret = OutputBuffer_put(pOutputBuffer, pBuffer, rlen) ) < 0){
				warn_msg(ret,"TS write failed");
			}
		}else{
			static unsigned  countVerboseMsg = 0;
			countVerboseMsg ++;
			usleep(250000);
			if((args.flags & 0x1) && 3 <= countVerboseMsg ) {  //# verbose: status message
				unsigned statData[2];
				countVerboseMsg = 0;
				ret = tc90522_readStatistic(pDemod, tunerNum, statData);
				if(! ret) {
					//# sucess
					if(!(statData[0] & 0x10))  msg("Status=%02X, ", statData[0]);
					msg("CNR= %4dmB, ", (int32_t)statData[1] );
				}
				msg("\n");
			}
		}
	}
	ret = 0;

	//# restore signal handler
	setSignalHandler(0, sighandler);
	//# flush data stream
	tsthread_stop(tsthr);
	if( pOutputBuffer ) {
		OutputBuffer_flush(pOutputBuffer);
		OutputBuffer_release(pOutputBuffer);
	}
	//# time of the end of rec.
	u_gettime(&time_end);
end6:
	tsthread_destroy(tsthr);
end5:
	if(tunerNum==0){
		mxl136_destroy(pTuner[0]);
	}else{
		tda20142_destroy(pTuner[1]);
	}
end4:
	tc90522_destroy(pDemod);
end3:
#ifdef DB_CARDREADER
	em287x_destroy(devst, dev_flg);
#else
	em287x_destroy(devst);
#endif
//end2:
	if(close(usbep.fd) == -1) {
		warn_msg(errno,"failed to close usbdevfile");
	}
end1:
	if(0 == ret) {
		u_difftime(&time_begin, &time_end, &i, &j);
		msg("\n# Done! %u.%03u sec.\n", i, j);
	}else{
		warn_msg(ret,"\n# Abort!");
	}

	return ret;
}


static void printTMCC(void* const pDemod, const unsigned tunerNum)
{
	const char * const s_carmode[7] = {"DQPSK","QPSK","16QAM","64QAM","BPSK","TC8PSK","??"};
	const char * const s_crmode[6] = {"1/2","2/3","3/4","5/6","7/8","??"};
	uint8_t bf[28], *ptr = bf, j;
	
	if( tc90522_readTMCC( pDemod, tunerNum, bf ) )  return;
	if( bf[0] ) {
		msg("# TMCC: TXmode=%u GuardInterval=1/%u OneSeg=%c\n", bf[0], bf[1], (bf[2])? 'Y':'N' );
		for( j = 0; j < 3; j++ ) {
			ptr += 4;
			if(! ptr[0])  continue;
msg("%c: %-6s %2useg FEC=%-3s IL=%-2u\n", j + 'A', s_carmode[ ptr[1] ], ptr[0], s_crmode[ ptr[2] ], ptr[3]);
		}
	}else{
		const char s_modeIndex[] = "HLabcd";
		const unsigned ts_id = (bf[2] << 8) | bf[3];
		msg("# TMCC: TSID= %u (0x%04X)\n", ts_id, ts_id);
		for( j = 0; j < 6; j++ ) {
			ptr += 4;
			if(! ptr[0])  continue;
msg("%c: %-6s %2uslot FEC=%-3s\n", s_modeIndex[j], s_carmode[ ptr[1] ], ptr[0], s_crmode[ ptr[2] ] );
		}
	}
}



/*EOF*/
