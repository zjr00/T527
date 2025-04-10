Target = peerconnet

APP_PATH=.

THIRD_PARTY_PATH=../include


CC = gcc
CPP = g++
STRIP = strip

#SRCS := $(shell find -name "*.cc")
#SRCS += $(shell find -name "*.c")

COBJS := $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJS := $(patsubst %.cc, %.o, $(wildcard *.cc))

CFLAGS   += -g -O0 -pipe -Wall -W -Wl,--no-undefined -Wno-unused-parameter -Wno-unused-function \
			-Wno-write-strings -Wmissing-field-initializers -Wformat=0 -fPIC

CXXFLAGS += -g -fPIC -DWEBRTC_POSIX --std=gnu++14 -Wno-deprecated-declarations \
	-DUSE_UDEV -DUSE_AURA=1 -DWEBRTC_USE_X11 -DUSE_GLIB=1 -DUSE_NSS_CERTS=1 -DUSE_OZONE=1 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS -D_FORTIFY_SOURCE=2 -DCR_SYSROOT_HASH=95051d95804a77144986255f534acb920cee375b -DNDEBUG -DNVALGRIND -DDYNAMIC_ANNOTATIONS_ENABLED=0 \
	-DWEBRTC_ENABLE_PROTOBUF=0 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DRTC_ENABLE_VP9 -DRTC_DAV1D_IN_INTERNAL_DECODER_FACTORY -DWEBRTC_HAVE_SCTP -DWEBRTC_USE_H264 \
	-DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0 -DWEBRTC_LINUX -DABSL_ALLOCATOR_NOTHROW=1 -DABSL_FLAGS_STRIP_NAMES=0 -DHAVE_WEBRTC_VIDEO \
	-fno-aligned-new -Wno-narrowing -Wno-class-memaccess -fno-exceptions	-fvisibility-inlines-hidden -Wnon-virtual-dtor -Woverloaded-virtual \
	-Wno-deprecated-declarations -Wno-undef \
	-Wl,--fatal-warnings -Wl,--build-id -fPIC -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -fuse-ld=gold \
	-Wl,--threads -Wl,--thread-count=4 -m64 -Wl,-O2 -Wl,--gc-sections -rdynamic -Wl,-z,defs -Wl,--as-needed -pie -Wl,--disable-new-dtags \

INCLUDES += -I$(APP_PATH) \
			-I $(THIRD_PARTY_PATH)			\
			-I $(THIRD_PARTY_PATH)/api				\
			-I $(THIRD_PARTY_PATH)/third_party/libyuv/include			\
			-I $(THIRD_PARTY_PATH)/third_party/abseil-cpp			\
			-I $(THIRD_PARTY_PATH)/third_party/harfbuzz-ng/src/src \
			-I $(THIRD_PARTY_PATH)/build/gtk-3.0		\
			-I $(THIRD_PARTY_PATH)/build/atk-1.0		\
			-I $(THIRD_PARTY_PATH)/build/gdk-pixbuf-2.0 \
			-I $(THIRD_PARTY_PATH)/build/glib-2.0		\
			-I $(THIRD_PARTY_PATH)/build/cairo	\
			-I $(THIRD_PARTY_PATH)/build/pango-1.0	\
			-I $(THIRD_PARTY_PATH)/build/graphene-1.0	\
			-I /usr/include \
			
#			-I /home/zhang/WbRTC/code/src/build/linux/debian_sid_amd64-sysroot/usr/include \
		
LDFLAGS += -L $(THIRD_PARTY_PATH)/../lib \
	-L /home/zhang/WbRTC/code/src/build/linux/debian_sid_amd64-sysroot/lib/x86_64-linux-gnu \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/rtc_base \
	-L $(THIRD_PARTY_PATH)/../lib/test \
	-L $(THIRD_PARTY_PATH)/../lib/third_party \
	-L $(THIRD_PARTY_PATH)/../lib/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/modules/desktop_capture\
	-L /home/zhang/WbRTC/code/src/out/Release/obj/third_party/boringssl \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/third_party/jsoncpp \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/third_party/libyuv \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/media \
	-L /home/zhang/WbRTC/code/src/out/Release/obj/api/task_queue \
	

	
LIBRARIES += -ljsoncpp -lrtc_json -lboringssl -lwebrtc  -lrtc_base -lrtc_base_approved  -lthreading -ltask_queue  -lplatform_video_capturer  -lrtc_media_base -lparse \
		 -lpthread -lXrandr -lXdamage -lXfixes -lm -lz\
		-lX11 -ldesktop_capture_generic -lXcomposite -lXext -lXrender -ldl  -lyuv_internal -lXtst -lgbm\
		-lglib-2.0 -lrt -lgmodule-2.0 -lgthread-2.0 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lgtk-3\
		-lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -latomic \
		 
	
all : install

${Target}: $(COBJS) $(CPPOBJS) $(EXT_OBJS)
	$(CPP) -o ${Target} $(COBJS) $(CPPOBJS) $(LDFLAGS) $(LIBRARIES)
	@chmod 755 ${Target}
	@echo "generate $(Target) success!!!"
	
$(COBJS) : %.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o $@ -c $<

$(CPPOBJS) : %.o: %.cc
	$(CPP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -o $@ -c $<
	
.PHONY:clean cleanall install distclean

strip:
	$(STRIP) $(Target)

clean:
	-rm -f $(Target) $(COBJS) $(CPPOBJS)
	
install: $(Target)
