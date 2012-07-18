/*******************************************************************************
#                                                                              #
#                                                                              #		
#      Copyright 2012 Mark Majeres (slug_)  mark@engine12.com	               #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/

#include "v4l2ServerMediaSubsession.h"
#include "speexFromPCMAudioSource.h"

#include "JPEGVideoRTPSink.hh"
#include "v4l2JPEGDeviceSource.hh"

//#define CONVERT_TO_ULAW
//#define CONVERT_TO_SPEEX

videoServerMediaSubsession* videoServerMediaSubsession::createNew(UsageEnvironment& env, Boolean reuseFirstSource) {
	return new videoServerMediaSubsession(env, reuseFirstSource);
}

videoServerMediaSubsession::videoServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource)
					: OnDemandServerMediaSubsession(env, reuseFirstSource) {
}

videoServerMediaSubsession::~videoServerMediaSubsession() {
}

FramedSource* videoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {

	estBitrate = 500; // kbps
	return v4l2JPEGDeviceSource::createNew(envir());
}


RTPSink* videoServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
								  unsigned char rtpPayloadTypeIfDynamic,
								  FramedSource* /*inputSource*/) {
	return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
}



//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

audioServerMediaSubsession* audioServerMediaSubsession::createNew(UsageEnvironment& env, Boolean reuseFirstSource) {
	return new audioServerMediaSubsession(env, reuseFirstSource);
}

audioServerMediaSubsession::audioServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource)
					: OnDemandServerMediaSubsession(env, reuseFirstSource) {
}

audioServerMediaSubsession::~audioServerMediaSubsession() {
}

FramedSource* audioServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {

	
				// Create an appropriate RTP sink from the RTP 'groupsock':
	unsigned timePerFrame = 20;//1000000/30; // in microseconds (at 30 fps)
	unsigned const averageFrameSizeInBytes = 500;//35000; // estimate
	const unsigned estimatedSessionBandwidthAudio = (8*1000*averageFrameSizeInBytes)/timePerFrame;
	
	
	estBitrate = estimatedSessionBandwidthAudio;//15; // kbps
	
	
	
//	return v4l2JPEGDeviceSource::createNew(envir());
#if 1

//	if (!fork())
	{
						
		if(system("killall speexenc") != 0){}			
						
		if(system("rm /tmp/audio 2>/dev/null; mkfifo /tmp/audio 2>/dev/null") != 0)
		{
			*env << "Failed to make buffer \n";
			return 0;
		}
		
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE >> /tmp/audio  &") != 0)
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc -w --quality 2 - /tmp/audio &") != 0)
		if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc >> /tmp/audio & ") != 0)
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc -w --vad --dtx --quality 2 - /tmp/audio &") != 0)
		{
			*env << "Unable to open audio source \n";
	//		return 0;
		}
		
		
	}
#endif

	unsigned int preferredFrameSize = 0;//1500;//43;//1500;//160;
					 
//	ByteStreamFileSource* pcmSource = ByteStreamFileSource::createNew(*env, "/tmp/audio", preferredFrameSize, playTimePerFrame );
	speexFromPCMAudioSource* pcmSource = speexFromPCMAudioSource::createNew(*env, "/tmp/audio", preferredFrameSize);
	
	if (pcmSource == NULL) {
		envir() << "Unable to open stdin as a WAV audio file source:"
		 << "  "<< envir().getResultMsg() << "\n";
		return 0;
	}
	
	FramedSource* audiosource = pcmSource;
//	samplingFrequency = pcmSource->samplingFrequency();
	samplingFrequency = 16000;
//	numChannels = pcmSource->numChannels();
	numChannels = 1;

	/*	audiosource = EndianSwap16::createNew(envir(), pcmSource);
		if (audiosource == NULL) {
			envir() << "Unable to create a little->bit-endian order filter from the PCM audio source: "
			<< envir().getResultMsg() << "\n";
			return 0;
		}
	*/
	mimeType = "SPEEX";	
	payloadFormatCode = 96; // a dynamic RTP payload type

	return audiosource;
}


RTPSink* audioServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
								  unsigned char rtpPayloadTypeIfDynamic,
								  FramedSource* inputSource) {
//	return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
	
	return SimpleRTPSink::createNew(*env, rtpGroupsock,
									payloadFormatCode, samplingFrequency,
									"audio", mimeType, numChannels);
														


}

