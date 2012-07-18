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

#ifndef _V4L2_JPEG_DEVICE_SOURCE_HH
#define _V4L2_JPEG_DEVICE_SOURCE_HH

#include "JPEGVideoSource.hh"

class v4l2JPEGDeviceSource: public JPEGVideoSource {
public:
	static v4l2JPEGDeviceSource* createNew(UsageEnvironment& env);
  
protected:
	v4l2JPEGDeviceSource(UsageEnvironment& env);
	virtual ~v4l2JPEGDeviceSource();

private:
	virtual void doGetNextFrame();
	virtual u_int8_t type();
	virtual u_int8_t qFactor();
	virtual u_int8_t width();
	virtual u_int8_t height();

private:
	int setParamsFromHeader(unsigned char* data, unsigned int data_size);
	
private:
	u_int8_t fLastWidth, fLastHeight;

};


#endif
