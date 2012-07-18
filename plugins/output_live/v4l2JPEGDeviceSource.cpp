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

#include "v4l2JPEGDeviceSource.hh"
#include <sys/ioctl.h>
#include <time.h>

v4l2JPEGDeviceSource* v4l2JPEGDeviceSource::createNew(UsageEnvironment& env) 
{
  return new v4l2JPEGDeviceSource(env);
}

v4l2JPEGDeviceSource::v4l2JPEGDeviceSource(UsageEnvironment& env):JPEGVideoSource(env) 
{

}

v4l2JPEGDeviceSource::~v4l2JPEGDeviceSource() {
  
}


void v4l2JPEGDeviceSource::doGetNextFrame() {

static unsigned char *frame = NULL;
static unsigned int max_frame_size=0;
	
	gettimeofday(&fPresentationTime, NULL);


	//output_file
	
	unsigned char *tmp_framebuffer = NULL;

	pthread_mutex_lock(&pglobal->in[input_number].db);
	pthread_cond_wait(&pglobal->in[input_number].db_update, &pglobal->in[input_number].db);

	// read buffer
	unsigned int frame_size = pglobal->in[input_number].size;


	if(frame_size > max_frame_size) {
		DBG("increasing buffer size to %d\n", frame_size);

		max_frame_size = frame_size + (1 << 16);
		if((tmp_framebuffer = (unsigned char *)::realloc(frame, max_frame_size)) == NULL) {
			pthread_mutex_unlock(&pglobal->in[input_number].db);
			LOG("not enough memory\n");
			return;
		}

		frame = tmp_framebuffer;
	}

	// copy frame to our local buffer now 
	memmove(frame, pglobal->in[input_number].buf, frame_size);
	
	// allow others to access the global buffer again 
	pthread_mutex_unlock(&pglobal->in[input_number].db);

	int sz_Header = setParamsFromHeader(frame,frame_size);

	unsigned int szPayload=frame_size-sz_Header;

	if (szPayload > fMaxSize) {
		fprintf(stderr, "v4l2JPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n", fMaxSize);
		szPayload = fMaxSize;
	}
	memmove(fTo, frame+sz_Header, szPayload);
	fFrameSize = szPayload;
	
		// Switch to another task, and inform the reader of the new data:
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)FramedSource::afterGetting, this);
}


u_int8_t v4l2JPEGDeviceSource::type() 
{
  return 1;
}

u_int8_t v4l2JPEGDeviceSource::qFactor() 
{
//	fprintf(stdout, "v4l2JPEGDeviceSource::qFactor(): %d\n", fLastQFactor);
	return server.quality;
}

u_int8_t v4l2JPEGDeviceSource::width() 
{
//	fprintf(stdout, "v4l2JPEGDeviceSource::width(): %d\n", fLastWidth);	
	return fLastWidth;
}

u_int8_t v4l2JPEGDeviceSource::height() 
{
//	fprintf(stdout, "v4l2JPEGDeviceSource::height(): %d\n", fLastHeight);	
	return fLastHeight;
}


//Gets the JPEG size from the array of data passed to the function 
//and returns the size of the JPEG header in bytes
int v4l2JPEGDeviceSource::setParamsFromHeader(unsigned char* data, unsigned int data_size) {
	//Check for valid JPEG image
	unsigned int i=0;
	if(data[i] == 0xFF && data[i+1] == 0xD8 && data[i+2] == 0xFF && data[i+3] == 0xE0) {
		i += 4;
		// Check for valid JPEG header (null terminated JFIF)
		//if(data[i+2] == 'J' && data[i+3] == 'F' && data[i+4] == 'I' && data[i+5] == 'F' && data[i+6] == 0x00) {
			//Retrieve the block length of the first block since the first block will not contain the size of file
			i+=(data[i] * 256 + data[i+1]);
			while(i<data_size) {
		
				if(data[i] != 0xFF) break;   //Check that we are truly at the start of another block
				if(data[i+1] == 0xC0) {            //0xFFC0 is the "Start of frame" marker which contains the file size
					fLastHeight = (data[i+5]<<5)|(data[i+6]>>3);
					fLastWidth = (data[i+7]<<5)|(data[i+8]>>3);
				}
				//Increase the file index to get to the next block
				i+=2;                              //Skip the block marker
				i+=(data[i] * 256 + data[i+1]);   //Go to the next block
			}
		 
		//}                  
         
	}
	else
		return -1;                     //Not a valid SOI header
	
	return i;
	
}