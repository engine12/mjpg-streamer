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
// C++ header

#ifndef _SPEEXBYTE_STREAM_FILE_SOURCE_HH
#define _SPEEXBYTE_STREAM_FILE_SOURCE_HH

#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif



extern "C"{

#include <speex/speex.h>
//#include <ogg/ogg.h>

}

class speexFromPCMAudioSource: public FramedFileSource {
public:
  static speexFromPCMAudioSource* createNew(UsageEnvironment& env,
					 char const* fileName,
					 unsigned preferredFrameSize = 0);
  // "preferredFrameSize" == 0 means 'no preference'
  // "playTimePerFrame" is in microseconds

  static speexFromPCMAudioSource* createNew(UsageEnvironment& env,
					 FILE* fid,
					 unsigned preferredFrameSize = 0);
      // an alternative version of "createNew()" that's used if you already have
      // an open file.

  u_int64_t fileSize() const { return fFileSize; }
      // 0 means zero-length, unbounded, or unknown
/*
  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
    // if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteRelative(int64_t offset);
  void seekToEnd(); // to force EOF handling on the next read
*/
protected:
  speexFromPCMAudioSource(UsageEnvironment& env,
		       FILE* fid,
		       unsigned preferredFrameSize);
	// called only by createNew()

  virtual ~speexFromPCMAudioSource();

  static void fileReadableHandler(speexFromPCMAudioSource* source, int mask);
  void doReadFromFile();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

protected:
  u_int64_t fFileSize;


  unsigned fPreferredFrameSize;


  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
  
  
  	int fd;

	SpeexBits bits;		//Speex bit-packing struct
	void *enc_state;	//Speex encoder state
	

	
	
};

#endif
