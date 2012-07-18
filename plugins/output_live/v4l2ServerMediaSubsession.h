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


#ifndef V4L2SERVERMEDIASUBSESSION_H
#define V4L2SERVERMEDIASUBSESSION_H

class videoServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static videoServerMediaSubsession*
	createNew(UsageEnvironment& env, Boolean reuseFirstSource);

private:
	videoServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource);
	virtual ~videoServerMediaSubsession();

private: 
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
										unsigned char rtpPayloadTypeIfDynamic,
										FramedSource* inputSource);
};


class audioServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static audioServerMediaSubsession*
	createNew(UsageEnvironment& env, Boolean reuseFirstSource);

private:
	audioServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource);
	virtual ~audioServerMediaSubsession();

private: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
										unsigned char rtpPayloadTypeIfDynamic,
										FramedSource* inputSource);
									
	char const* mimeType;
	unsigned char payloadFormatCode;
	unsigned int samplingFrequency;
	unsigned char numChannels;
};

#endif // V4L2SERVERMEDIASUBSESSION_H
