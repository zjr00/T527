
APP_PATH=.

TARGET_MODE = 0

THIRD_PARTY_PATH=./p2p/include

EXCLUDE_CPP_FILES = g2d.cpp

ifeq ($(TARGET_MODE), 1)

COBJS := $(patsubst %.c, %.o, $(wildcard *.c))
CPP_SRC = $(filter-out $(EXCLUDE_CPP_FILES), $(wildcard *.cpp))
CPPOBJS := $(patsubst %.cpp, %.o, $(CPP_SRC))

   Target = server
   CC = gcc
   CPP = g++
   STRIP = strip

LDFLAGS += -L ./osip/lib \
	-L /home/zhang/WbRTC/code/src/build/linux/debian_sid_amd64-sysroot/lib/x86_64-linux-gnu \
	-L $(THIRD_PARTY_PATH)/../lib/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/modules/desktop_capture\
	-L /mnt/hgfs/WebRTC/lib \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/rtc_base \
	-L $(THIRD_PARTY_PATH)/../lib/test \
	-L $(THIRD_PARTY_PATH)/../lib/third_party \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/third_party/jsoncpp \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/third_party/libyuv \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/media \
	-L /home/zhang/WbRTC/code/src/out/Release_nogtk/obj/api/task_queue \

else

COBJS := $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJS := $(patsubst %.cpp, %.o, $(wildcard *.cpp))

   Target = none_server
   CC = aarch64-none-linux-gnu-gcc
   CPP = aarch64-none-linux-gnu-g++
   STRIP = aarch64-none-linux-gnu-strip

DEFINES += -DHAVE_PTHREADS -DHAVE_SYS_UIO_H -DANDROID_SMP=1 -D__ARM_HAVE_DMB -D__ARM_HAVE_LDREX_STREX -DHAVE_POSIX_CLOCKS -DHAVE_PRCTL \
	-DCDX_V27 -DSUPPORT_NEW_DRIVER -D_REENTRANT -DUSE_LOGCAT -DLOG_LEVEL=6  \
   
LDFLAGS +=  -L ./g2d/lib \
	-L ./osip/lib_none \
	-L /home/zhang/WbRTC/code/src/build/linux/debian_sid_amd64-sysroot/lib/x86_64-linux-gnu \
	-L $(THIRD_PARTY_PATH)/../none_lib/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/none/obj/modules/desktop_capture\
	-L /mnt/hgfs/WebRTC/lib \
	-L /home/zhang/WbRTC/code/src/out/none/obj/rtc_base \
	-L $(THIRD_PARTY_PATH)/../none_lib/test \
	-L $(THIRD_PARTY_PATH)/../none_lib/third_party \
	-L /home/zhang/WbRTC/code/src/out/none/obj/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/none/obj/third_party/jsoncpp \
	-L /home/zhang/WbRTC/code/src/out/none/obj/third_party/libyuv \
	-L /home/zhang/WbRTC/code/src/out/none/obj/third_party/libjpeg_turbo \
	-L /home/zhang/WbRTC/code/src/out/none/obj/media \
	-L /home/zhang/WbRTC/code/src/out/none/obj/api/task_queue \
	
LIBRARIES += -lsdk_g2d -lsdk_memory  -l:libjpeg.so.8.2.2 -lsdk_log -lMemAdapter -lVE -lcdc_base

endif



CFLAGS   += -g -O0 -pipe -Wall -W -Wl,--no-undefined -Wno-unused-parameter -Wno-unused-function \
			-Wno-write-strings -Wmissing-field-initializers -Wformat=0 -fPIC

CXXFLAGS += -g -fPIC -DWEBRTC_POSIX --std=gnu++14 -Wno-deprecated-declarations -std=c++17\
	-DUSE_UDEV -DUSE_AURA=1 -DUSE_GLIB=1 -DUSE_NSS_CERTS=1 -DUSE_OZONE=1 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS -D_FORTIFY_SOURCE=2 -DCR_SYSROOT_HASH=95051d95804a77144986255f534acb920cee375b -DNDEBUG -DNVALGRIND -DDYNAMIC_ANNOTATIONS_ENABLED=0 \
	-DWEBRTC_ENABLE_PROTOBUF=0 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DRTC_ENABLE_VP9 -DRTC_DAV1D_IN_INTERNAL_DECODER_FACTORY -DWEBRTC_HAVE_SCTP -DWEBRTC_USE_H264 \
	-DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0 -DWEBRTC_POSIX -DWEBRTC_LINUX -DABSL_ALLOCATOR_NOTHROW=1 -DABSL_FLAGS_STRIP_NAMES=0 \
	-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_40 -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_40 -DHAVE_WEBRTC_VIDEO -DWEBRTC_USE_X11 \
	-fno-aligned-new -Wno-narrowing -Wno-class-memaccess -fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual \
	-Wno-deprecated-declarations -Wno-undef \
	-Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -fuse-ld=gold \
	-Wl,--threads -Wl,--thread-count=4 -Wl,-O2 -Wl,--gc-sections -rdynamic -Wl,-z,defs -Wl,--as-needed -pie -Wl,--disable-new-dtags \


INCLUDES += -I $(APP_PATH) \
	-I $(APP_PATH)/osip/include \
	-I $(THIRD_PARTY_PATH)			\
	-I $(THIRD_PARTY_PATH)/api				\
	-I $(THIRD_PARTY_PATH)/third_party/libyuv/include			\
	-I $(THIRD_PARTY_PATH)/third_party/abseil-cpp			\
	-I $(THIRD_PARTY_PATH)/third_party/libjpeg_turbo \
	-I $(THIRD_PARTY_PATH)/third_party/harfbuzz-ng/src/src \
	-I $(THIRD_PARTY_PATH)/build/gtk-3.0		\
	-I $(THIRD_PARTY_PATH)/build/atk-1.0		\
	-I $(THIRD_PARTY_PATH)/build/gdk-pixbuf-2.0 \
	-I $(THIRD_PARTY_PATH)/build/glib-2.0		\
	-I $(THIRD_PARTY_PATH)/build/cairo	\
	-I $(THIRD_PARTY_PATH)/build/pango-1.0	\
	-I $(THIRD_PARTY_PATH)/build/graphene-1.0	\
	-I $(THIRD_PARTY_PATH)/third_party/jsoncpp/source/include	\
	-I ./g2d/include \
	-I ./g2d/include/storage \
	-I ./g2d/include/disp2 \
	-I ./g2d/include/cutils \
	-I ./g2d/include/camera \




	
LIBRARIES += -lpthread -leXosip2 -losip2 -losipparser2 -lresolv \
	 -ljsoncpp -lrtc_json -lboringssl -lwebrtc  -lrtc_base -lrtc_base_approved  -lthreading -ltask_queue  \
	-lrtc_media_base \
	-lpthread -lm -ldl\


all : install

${Target}: $(COBJS) $(CPPOBJS) $(EXT_OBJS)
	$(CPP) -o ${Target} $(COBJS) $(CPPOBJS) $(LDFLAGS) $(LIBRARIES)
	@chmod 755 ${Target}
	@echo "generate $(Target) success!!!"
	
$(COBJS) : %.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o $@ -c $<

$(CPPOBJS) : %.o: %.cpp
	$(CPP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -o $@ -c $<
	
.PHONY:clean cleanall install distclean

strip:
	$(STRIP) $(Target)

clean:
	-rm -f $(Target) $(COBJS) $(CPPOBJS)
	
install: $(Target)
