###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC = gcc
CXX = g++
OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

LIVE_LIBS_ROOT=/home/mark/Downloads/ZipIt/projectgus-openwrt-zipit-4ae4cea/staging_dir/target-arm_v5te_uClibc-0.9.32_eabi/usr/lib/live

TARGET_CFLAGS+=-I $(LIVE_LIBS_ROOT)/liveMedia/include
TARGET_CFLAGS+=-I $(LIVE_LIBS_ROOT)/groupsock/include
TARGET_CFLAGS+=-I $(LIVE_LIBS_ROOT)/UsageEnvironment/include
TARGET_CFLAGS+=-I $(LIVE_LIBS_ROOT)/BasicUsageEnvironment/include
#TARGET_LDFLAGS+=-lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock
#TARGET_LDFLAGS+=-L $(LIVE_LIBS_ROOT)/liveMedia/

LIB_SUFFIX=a
USAGE_ENVIRONMENT_DIR = ../UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = ../BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = ../liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = ../groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
SPEEX_DIR=../speex
SPEEX_LIB=$(SPEEX_DIR)/libspeex.$(LIB_SUFFIX)

LOCAL_LIBS=$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) $(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB) $(SPEEX_LIB)

CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC  -logg
#CFLAGS += -Os -pipe -march=armv5te -mtune=xscale -fno-caller-saves -fhonour-copts -msoft-float -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DLOCALE_NOT_USED

#CFLAGS += -DDEBUG
LFLAGS += -lpthread -ldl

all: output_live.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

output_live.so: $(OTHER_HEADERS) output_live.c
	$(CXX) $(CFLAGS) $(TARGET_CFLAGS) -o $@ output_live.c $(LOCAL_LIBS) 
