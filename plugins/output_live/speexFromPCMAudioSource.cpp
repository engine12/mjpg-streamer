/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2012 Live Networks, Inc.  All rights reserved.
// A file source that is a plain byte stream (rather than frames)
// Implementation

#include "speexFromPCMAudioSource.h"
#include "InputFile.hh"
#include "GroupsockHelper.hh"


#include "speexFromPCMAudioSource.h"
#include <speex/speex_stereo.h>
#include <speex/speex_preprocess.h>

//#define ENCODE_W_SPEEX
//#define SAVE_STREAM_TOFILE
////////// speexFromPCMAudioSource //////////

speexFromPCMAudioSource*
speexFromPCMAudioSource::createNew(UsageEnvironment& env, char const* fileName,
				unsigned preferredFrameSize) {
  FILE* fid = OpenInputFile(env, fileName);
  if (fid == NULL) return NULL;

  speexFromPCMAudioSource* newSource
    = new speexFromPCMAudioSource(env, fid, preferredFrameSize);
  newSource->fFileSize = GetFileSize(fileName, fid);

  return newSource;
}

speexFromPCMAudioSource*
speexFromPCMAudioSource::createNew(UsageEnvironment& env, FILE* fid,
				unsigned preferredFrameSize) {
  if (fid == NULL) return NULL;

  speexFromPCMAudioSource* newSource = new speexFromPCMAudioSource(env, fid, preferredFrameSize);
  newSource->fFileSize = GetFileSize(NULL, fid);

  return newSource;
}

/*
void speexFromPCMAudioSource::seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream) {
  SeekFile64(fFid, (int64_t)byteNumber, SEEK_SET);

  fNumBytesToStream = numBytesToStream;
  fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void speexFromPCMAudioSource::seekToByteRelative(int64_t offset) {
  SeekFile64(fFid, offset, SEEK_CUR);
}

void speexFromPCMAudioSource::seekToEnd() {
  SeekFile64(fFid, 0, SEEK_END);
}
*/

speexFromPCMAudioSource::speexFromPCMAudioSource(UsageEnvironment& env, FILE* fid,
					   unsigned preferredFrameSize)
  : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0) {
//#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  makeSocketNonBlocking(fileno(fFid));
//#endif



#ifdef SAVE_STREAM_TOFILE		
  
  		if((fd = open("/tmp/test1.spx", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		//    OPRINT("could not open the file \n");
			env << "could not open the file \n";
			return;
		}
#endif	
	
#ifdef ENCODE_W_SPEEX		
	int speex_quality = 2; //integer value ranging from 0 to 10 

	enc_state = speex_encoder_init(&speex_wb_mode);


	 
	speex_encoder_ctl(enc_state,SPEEX_SET_QUALITY,&speex_quality);
	speex_bits_init(&bits);
#endif	

		
}

speexFromPCMAudioSource::~speexFromPCMAudioSource() {
  if (fFid == NULL) return;

//#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
//#endif

  CloseInputFile(fFid);
  
#ifdef SAVE_STREAM_TOFILE
     	::close(fd);
#endif	

	
#ifdef ENCODE_W_SPEEX	
	//After you’re done with the encoding, free all resources with:
	speex_encoder_destroy(enc_state);
	speex_bits_destroy(&bits);	
#endif	
	
}

void speexFromPCMAudioSource::doGetNextFrame() {
  if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
    handleClosure(this);
    return;
  }

//#ifdef READ_FROM_FILES_SYNCHRONOUSLY
 // doReadFromFile();
//#else
  if (!fHaveStartedReading) {
    // Await readable data from the file:
    envir().taskScheduler().turnOnBackgroundReadHandling(fileno(fFid),
	       (TaskScheduler::BackgroundHandlerProc*)&fileReadableHandler, this);
    fHaveStartedReading = True;
  }
//#endif
}

void speexFromPCMAudioSource::doStopGettingFrames() {
//#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
  fHaveStartedReading = False;
  
  	if(system("killall speexenc") != 0)
		*env << "Failed to make buffer \n";

//#endif
}

void speexFromPCMAudioSource::fileReadableHandler(speexFromPCMAudioSource* source, int /*mask*/) {
  if (!source->isCurrentlyAwaitingData()) {
    source->doStopGettingFrames(); // we're not ready for the data yet
    return;
  }
  source->doReadFromFile();
}
/*
static Boolean const readFromFilesSynchronously
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
= True;
#else
= False;
#endif
*/
//#define FRAME_SIZE 160


#if 0
void speexFromPCMAudioSource::doReadFromFile() {
  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }

    fFrameSize = read(fileno(fFid), fTo, fMaxSize);

  if (fFrameSize == 0) {
    handleClosure(this);
    return;
  }
  fNumBytesToStream -= fFrameSize;

		printf("fMaxSize:= %d \n",fMaxSize);
		printf("fFrameSize:= %d \n",fFrameSize);
		
		
    gettimeofday(&fPresentationTime, NULL);

  // Because the file read was done from the event loop, we can call the
  // 'after getting' function directly, without risk of infinite recursion:
  FramedSource::afterGetting(this);

}
#else

#define FRAME_SIZE_SPX 320	
#define FRAMES_PER_RTP_PACKET 21
static __kernel_suseconds_t lasttime;
	
//#define FRAME_SIZE_ME 160		
void speexFromPCMAudioSource::doReadFromFile() {
	// Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
	if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
		fMaxSize = (unsigned)fNumBytesToStream;
	}	
	if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize){
		fMaxSize = fPreferredFrameSize;
	}
	
//	if (readFromFilesSynchronously || fFidIsSeekable) 
//		fFrameSize = fread(fTo, 1, fMaxSize, fFid);
//		fFrameSize = fread(fTo, sizeof(short), FRAME_SIZE, fFid);
	//	fflush(fFid);
//	else 
		// For non-seekable files (e.g., pipes), call "read()" rather than "fread()", to ensure that the read doesn't block:
		
//		int fFrameSize=0;
//		unsigned char* ptrOut=fTo;

		
//		while(fFrameSize < fMaxSize-FRAME_SIZE){
	

//			nBytesRead = fread(ptrOut, 1, FRAME_SIZE, fFid);
		//	nBytesRead = read(fileno(fFid), ptrOut, FRAME_SIZE);





#ifndef ENCODE_W_SPEEX		
    fFrameSize = read(fileno(fFid), fTo, fMaxSize);	

		gettimeofday(&fPresentationTime, NULL);	
#else


		gettimeofday(&fPresentationTime, NULL);
		
	fFrameSize=0;


	char cbits[1500];
	
	short in[FRAME_SIZE_SPX];

	
	int nbBytes;
	
//	int audio_frame_size;
	int nFrames = 1;
	
	//frame_size will correspond to 20 ms when using 8, 16, or 32 kHz sampling rate. 
//	speex_encoder_ctl(enc_state,SPEEX_GET_FRAME_SIZE,&audio_frame_size);														
//	printf("audio_frame_size = %d \n", audio_frame_size);
		
	speex_bits_reset(&bits);
	int nRawAudio_Bytes =0;
	
	while(nFrames<=FRAMES_PER_RTP_PACKET){
		
	    nRawAudio_Bytes = read(fileno(fFid), in, FRAME_SIZE_SPX);

//		nRawAudio_Bytes = fread(in, sizeof(short), FRAME_SIZE_SPX, fFid);		
		if(nRawAudio_Bytes != FRAME_SIZE_SPX) 
			return;		
	
//		printf("nRawAudio_Bytes:= %d \n",nRawAudio_Bytes);

			
		speex_encode_int(enc_state, in, &bits);
		
	//	if(nRawAudio_Bytes>=FRAME_SIZE_SPX){
			nFrames ++;
	//		nRawAudio_Bytes =0;
	//	}		
	}
	
	do{
		void* fOutPtr = fTo+sizeof(int);	
		nbBytes = speex_bits_write(&bits, (char*)fOutPtr, 1500);
	//	printf ("nbBytes: %d\n", nbBytes);
	//	(*(int*)(fTo))= nbBytes;
		memcpy(fTo,&nbBytes,sizeof(int));
	//	fTo[0]=0x97;
	//	fTo[1]=0x1;
	
	}while(0);

//	memmove(fOutPtr, cbits, nbBytes);			
//	fOutPtr+=nbBytes;

	fFrameSize = nbBytes+sizeof(int);	
	
#endif	
						
	
/*
	__kernel_suseconds_t timePerFrame = fPresentationTime.tv_usec-lasttime;
	printf("timePerFrame= %d\n", timePerFrame);	
		
	lasttime = fPresentationTime.tv_usec;
*/
#ifdef SAVE_STREAM_TOFILE
			
	if(write(fd, fTo, fFrameSize) < 0) 
		perror("write()");
#endif
							
						
							
//		fFrameSize = read(fileno(fFid), fTo, fMaxSize);
		
	if (fFrameSize == 0) {
		handleClosure(this);
		return;
	}
	
//		printf("fMaxSize:= %d \n",fMaxSize);
//		printf("fFrameSize:= %d \n",fFrameSize);
//		printf("fTo:= 0x%X%X \n",fTo[1],fTo[0]);

	fNumBytesToStream -= fFrameSize;

	

	// Inform the reader that he has data:


//	printf("fDurationInMicroseconds:+ %d \n",fDurationInMicroseconds);
	// Because the file read was done from the event loop, we can call the
	// 'after getting' function directly, without risk of infinite recursion:
	FramedSource::afterGetting(this);


	
}
#endif
