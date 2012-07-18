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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>

#include <dirent.h>

#include "../../utils.h"
#include "../../mjpg_streamer.h"

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include <BasicUsageEnvironment.hh>

#define OUTPUT_PLUGIN_NAME "Live555 output plugin"

static pthread_t worker;
static globals *pglobal;


static int input_number = 0;



UsageEnvironment* env;

struct context{
    int port;
    char *credentials;
    bool bMultiCast;
	int quality;
	int tunnel;
	bool bStreamAudio;
	bool bStreamVideo;
} server;

// A structure to hold the state of the current session.
// It is used in the "afterPlaying()" function to clean up the session.
struct sessionState_t {
  FramedSource* audiosource;
  FramedSource* videosource;
  RTPSink* audioSink;
  RTPSink* videoSink;
  RTCPInstance* rtcpAudioInstance;
  RTCPInstance* rtcpVideoInstance;
  Groupsock* rtpGroupsockAudio;
  Groupsock* rtpGroupsockVideo;
  Groupsock* rtcpGroupsockAudio;
  Groupsock* rtcpGroupsockVideo;
  RTSPServer* rtspServer;
} sessionState;

#include "v4l2ServerMediaSubsession.cpp"
#include "v4l2JPEGDeviceSource.cpp"

//#define CONVERT_TO_SPEEX
//#ifdef CONVERT_TO_SPEEX
#include "speexFromPCMAudioSource.cpp"
//#endif

extern "C" int output_init(output_parameter *param);
extern "C" int output_stop(int id);
extern "C" int output_run(int id);
extern "C" int output_cmd(int plugin, unsigned int control_id, unsigned int group, int value);


// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = true;

/******************************************************************************
Description.: print a help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " ---------------------------------------------------------------\n" \
            " Help for output plugin..: "OUTPUT_PLUGIN_NAME"\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this plugin:\n\n" \
            " [-p | --port ]..........: port for this RTSP server\n" \
            " [-c | --credentials ]...: ask for \"username:password\" on connect\n" \
            " [-m | --multicast ]...: server will multicast (vs. unicast) -- no args\n" \
			" [-q | --quality ]......: JPEG compression quality in percent\n" \
			"                           should match the quality value given to input_uvc\n" \
			" [-t | --tunnel ]......: port number for RTSP-over-HTTP tunneling\n" \
			"                           disabled if not specified\n" \
			" [-a | --audio ]......: stream audio\n" \
			" [-v | --video ]......: stream video\n" \
            " ---------------------------------------------------------------\n");
}

/******************************************************************************
Description.: clean up allocated ressources
Input Value.: unused argument
Return Value: -
******************************************************************************/
void worker_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    if(!first_run) {
        DBG("already cleaned up ressources\n");
        return;
    }

    first_run = 0;
    OPRINT("cleaning up ressources allocated by worker thread\n");
}

void play(); // forward
void listenRTSP();
void afterPlaying(void* clientData); // forward
int prepareAudio();

/******************************************************************************
Description.: this is the main worker thread
              it loops forever, grabs a fresh frame and stores it to file
Input Value.:
Return Value:
******************************************************************************/
void *worker_thread(void *arg)
{
    /* set cleanup handler to cleanup allocated ressources */
    pthread_cleanup_push(worker_cleanup, NULL);

	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
	



	if(server.bMultiCast == true)
		play();
	else
		listenRTSP();
	
	//afterPlaying(NULL);
	
	if(system("killall speexenc") != 0){}
		
		
    /* cleanup now */
    pthread_cleanup_pop(1);

    return NULL;
}

/*** plugin interface functions ***/
/******************************************************************************
Description.: this function is called first, in order to initialize
              this plugin and pass a parameter string
Input Value.: parameters
Return Value: 0 if everything is OK, non-zero otherwise
******************************************************************************/
int output_init(output_parameter *param)
{
    int i;

    DBG("output #%02d\n", param->id);

    server.port = 8554;	
	server.credentials = NULL;
    server.bMultiCast = false;
	server.quality = 80;
	server.tunnel = -1;
	server.bStreamAudio = false;
	server.bStreamVideo = false;
	
    param->argv[0] = (char*)OUTPUT_PLUGIN_NAME;

    /* show all parameters for DBG purposes */
    for(i = 0; i < param->argc; i++) {
        DBG("argv[%d]=%s\n", i, param->argv[i]);
    }

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"p", required_argument, 0, 0},
            {"port", required_argument, 0, 0},
            {"c", required_argument, 0, 0},
            {"credentials", required_argument, 0, 0},
            {"m", no_argument, 0, 0},
            {"multicast", no_argument, 0, 0},
            {"q", required_argument, 0, 0},
            {"quality", required_argument, 0, 0},
            {"t", required_argument, 0, 0},
            {"tunnel", required_argument, 0, 0},            
            {"a", no_argument, 0, 0},
            {"audio", no_argument, 0, 0},
            {"v", no_argument, 0, 0},
            {"video", no_argument, 0, 0},
			{0, 0, 0, 0}
        };

        c = getopt_long_only(param->argc, param->argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return 1;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            DBG("case 0,1\n");
            help();
            return 1;
            break;

            /* p, port */
        case 2:
        case 3:
            DBG("case 2,3\n");
            server.port = atoi(optarg);
            break;

            /* c, credentials */
        case 4:
        case 5:
            DBG("case 4,5\n");
            server.credentials = strdup(optarg);
            break;

            /* m, multicast */
        case 6:
        case 7:
            DBG("case 6,7\n");
            server.bMultiCast = true;
            break;

            /* q, quality */
        case 8:
        case 9:
            DBG("case 8,9\n");
            server.quality = MIN(MAX(atoi(optarg), 0), 100);
            break;

            /* t, tunnel */
        case 10:
        case 11:
            DBG("case 10,11\n");
            server.tunnel = atoi(optarg);
			break;

            /* a, audio */
        case 12:
        case 13:
            DBG("case 12,13\n");
            server.bStreamAudio = true;
			break;

            /* v, video */
        case 14:
        case 15:
            DBG("case 14,15\n");
            server.bStreamVideo = true;
	        break;
        }
    }

    pglobal = param->global;
    if(!(input_number < pglobal->incnt)) {
        OPRINT("ERROR: the %d input_plugin number is too much only %d plugins loaded\n", input_number, pglobal->incnt);
        return 1;
    }

    OPRINT("MultiCast...: %s\n", (server.bMultiCast?"yes":"no"));
    OPRINT("RTSP port.....: %d\n",  server.port);
    OPRINT("username:password.: %s\n", (server.credentials == NULL) ? "disabled" : server.credentials);
	OPRINT("JPEG Quality......: %d\n", server.quality);
	OPRINT("RTSP-over-HTTP tunneling...: %s\n", ((server.tunnel ==-1)?"disabled":"enabled"));
    return 0;
}

/******************************************************************************
Description.: calling this function stops the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_stop(int id)
{
    DBG("will cancel worker thread\n");
    pthread_cancel(worker);
    return 0;
}


/******************************************************************************
Description.: calling this function creates and starts the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_run(int id)
{
    DBG("launching worker thread\n");
    pthread_create(&worker, 0, worker_thread, NULL);
    pthread_detach(worker);
    return 0;
}

int output_cmd(int plugin, unsigned int control_id, unsigned int group, int value)
{
    DBG("command (%d, value: %d) for group %d triggered for plugin instance #%02d\n", control_id, value, group, plugin);
    return 0;
}

//#define CONVERT_TO_ULAW

int prepareAudio()
{

	

//	return v4l2JPEGDeviceSource::createNew(envir());
#if 1

//	if (!fork())
	{
						
				
						
		if(system("rm /tmp/audio 2>/dev/null; mkfifo /tmp/audio 2>/dev/null") != 0)
		{
			*env << "Failed to make buffer \n";
			return 0;
		}
		
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE >> /tmp/audio  &") != 0)
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc -w --quality 2 - /tmp/audio &") != 0)
		if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc >> /tmp/audio &") != 0)
	//	if(system("arecord -D hw:1,0 -r 16000 -t raw -c 1 -f S16_LE | speexenc -w --vad --dtx --quality 2 - /tmp/audio &") != 0)
		{
			*env << "Unable to open audio source \n";
	//		return 0;
		}
		
		
	}
#endif
	
	return 0;														
}

void startMulticast(){
	
	// Create 'groupsocks' for RTP and RTCP:
	struct in_addr destinationAddress;
	destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);
	const unsigned char ttl = 255;
		
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
		  
	if(server.bStreamAudio){
	///////////////////  audio    ////////////////////////////////////////////
		const unsigned short rtpPortNumAudio = 6666;
		const unsigned short rtcpPortNumAudio = rtpPortNumAudio+1;		

		const Port rtpPortAudio(rtpPortNumAudio);
		const Port rtcpPortAudio(rtcpPortNumAudio);

		sessionState.rtpGroupsockAudio 	= new Groupsock(*env, destinationAddress, rtpPortAudio, ttl);
		sessionState.rtcpGroupsockAudio	= new Groupsock(*env, destinationAddress, rtcpPortAudio, ttl);
		
		sessionState.rtpGroupsockAudio->multicastSendOnly();
		sessionState.rtcpGroupsockAudio->multicastSendOnly();
		


	unsigned timePerFrame = 20;//1000000/30; // in microseconds (at 30 fps)
	unsigned const averageFrameSizeInBytes = 42;//35000; // estimate
	const unsigned estimatedSessionBandwidthAudio = (8*averageFrameSizeInBytes)/timePerFrame;
	
	
	prepareAudio();	
						
						
	sessionState.audioSink = SimpleRTPSink::createNew(*env, sessionState.rtpGroupsockAudio,
									96 /*payloadFormatCode*/, 160000 /*samplingFrequency*/,
									"audio", "SPEEX" /*mimeType*/, 1 /*numChannels*/);

	// Create (and start) a 'RTCP instance' for the audio sink:
	sessionState.rtcpAudioInstance = RTCPInstance::createNew(*env, sessionState.rtcpGroupsockAudio,
														estimatedSessionBandwidthAudio, CNAME,
														sessionState.audioSink, NULL /* we're a server */,
														True /* we're a SSM source*/);
				  
	}
				  
	if(server.bStreamVideo){
	///////////////////  video    ////////////////////////////////////////////
		const unsigned short rtpPortNumVideo = 8888;
		const unsigned short rtcpPortNumVideo = rtpPortNumVideo+1;

		const Port rtpPortVideo(rtpPortNumVideo);
		const Port rtcpPortVideo(rtcpPortNumVideo);

		sessionState.rtpGroupsockVideo 	= new Groupsock(*env, destinationAddress, rtpPortVideo, ttl);
		sessionState.rtcpGroupsockVideo	= new Groupsock(*env, destinationAddress, rtcpPortVideo, ttl);

		sessionState.rtpGroupsockVideo->multicastSendOnly();
		sessionState.rtcpGroupsockVideo->multicastSendOnly();
		
			// Create an appropriate RTP sink from the RTP 'groupsock':
		unsigned timePerFrame = 1000000/30; // in microseconds (at 30 fps)
	unsigned const averageFrameSizeInBytes = 35000; // estimate
	const unsigned estimatedSessionBandwidthVideo = (8*1000*averageFrameSizeInBytes)/timePerFrame;

	sessionState.videoSink = JPEGVideoRTPSink::createNew(*env, sessionState.rtpGroupsockVideo);

	// Create (and start) a 'RTCP instance' for the video sink:
	sessionState.rtcpVideoInstance = RTCPInstance::createNew(*env, sessionState.rtcpGroupsockVideo,
														  estimatedSessionBandwidthVideo, CNAME,
														  sessionState.videoSink, NULL /* we're a server */,
														  True /* we're a SSM source*/);

	}

  // Create and start a RTSP server to serve this stream:
	sessionState.rtspServer = RTSPServer::createNew(*env, server.port);
	if (sessionState.rtspServer == NULL) {
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		return;
	}
	ServerMediaSession* sms = ServerMediaSession::createNew(*env, "mjpg_streamer", "mjpg_streamer",
															"Session streamed by v4l2 camera", 
															True/*SSM*/);
	if(server.bStreamAudio)
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*sessionState.audioSink, sessionState.rtcpAudioInstance));

	if(server.bStreamVideo)
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*sessionState.videoSink, sessionState.rtcpVideoInstance));

	sessionState.rtspServer->addServerMediaSession(sms);

//	OutPacketBuffer::maxSize = 100000; // allow for some possibly large frames
   
	char* url = sessionState.rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;

}	



void play() {
	
	startMulticast();

	if(server.bStreamVideo)
	{	
		sessionState.videosource = v4l2JPEGDeviceSource::createNew(*env);	
		if (sessionState.videosource == NULL) {
			*env << "Unable to open camera: "
			 << env->getResultMsg() << "\n";
			return;
		}
	}

	if(server.bStreamAudio)
	{	
		unsigned int preferredFrameSize = 0;//1500;//43;//1500;//160;
		sessionState.audiosource = speexFromPCMAudioSource::createNew(*env, "/tmp/audio", preferredFrameSize);

		
		if (sessionState.audiosource == NULL) {
			*env << "Unable to open audio source: "
			 << env->getResultMsg() << "\n";
			return;
		}
	}
	
	// Finally, start the streaming:
	*env << "Beginning streaming...\n";
	if(server.bStreamAudio)
	sessionState.audioSink->startPlaying(*sessionState.audiosource, afterPlaying, sessionState.audioSink);

	if(server.bStreamVideo)
	sessionState.videoSink->startPlaying(*sessionState.videosource, afterPlaying, sessionState.videoSink);

  
	env->taskScheduler().doEventLoop((char*)&pglobal->stop);
}


void afterPlaying(void* /*clientData*/) {

  // One of the sinks has ended playing.
  // Check whether any of the sources have a pending read.  If so,
  // wait until its sink ends playing also:
  if (sessionState.audiosource->isCurrentlyAwaitingData()
      || sessionState.videosource->isCurrentlyAwaitingData()) return;

  // Now that both sinks have ended, close both input sources,
  // and start playing again:

		
	*env << "...done streaming\n";
	
	sessionState.audioSink->stopPlaying();
	sessionState.videoSink->stopPlaying();
  
	// End by closing the media:
	Medium::close(sessionState.rtspServer);
	Medium::close(sessionState.audioSink);
	Medium::close(sessionState.videoSink);
	delete sessionState.rtpGroupsockAudio;
	delete sessionState.rtpGroupsockVideo;
	Medium::close(sessionState.audiosource);
	Medium::close(sessionState.videosource);
	Medium::close(sessionState.rtcpAudioInstance);
	Medium::close(sessionState.rtcpVideoInstance);
	delete sessionState.rtcpGroupsockAudio;
	delete sessionState.rtcpGroupsockVideo;

}


void listenRTSP() {

	UserAuthenticationDatabase* authDB = NULL;

	if(server.credentials){
		//find the : delimiter in the user name and password string
		char* password = strchr(server.credentials, ':');
		if(password){
			// To implement client access control to the RTSP server, do the following:
			authDB = new UserAuthenticationDatabase;
			authDB->addUserRecord(server.credentials, password+1);
			// Repeat the above with each <username>, <password> that you wish to allow
			// access to the server.
		}
	}

	// Create the RTSP server:
	RTSPServer* rtspServer = RTSPServer::createNew(*env, server.port, authDB);
	if (rtspServer == NULL) {
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		return;
	}

	
	ServerMediaSession* sms = ServerMediaSession::createNew(*env, "mjpg_streamer", "mjpg_streamer",
															"Session streamed by v4l2 camera");
	if(server.bStreamVideo)
		sms->addSubsession(videoServerMediaSubsession::createNew(*env, reuseFirstSource));

	if(server.bStreamAudio)
		sms->addSubsession(audioServerMediaSubsession::createNew(*env, reuseFirstSource));
		
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;  


	// Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
	if ( server.tunnel!=-1){
		if(rtspServer->setUpTunnelingOverHTTP(server.tunnel))
			*env << "\nWe use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.\n";
		else 
			*env << "\n*** RTSP-over-HTTP tunneling is not available on port " << server.tunnel << " ***\n";
		
	}
	
	env->taskScheduler().doEventLoop(); // does not return

}
