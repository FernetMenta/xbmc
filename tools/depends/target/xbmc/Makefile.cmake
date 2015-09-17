-include ../../Makefile.include

VERSION.TXT := $(XBMCROOT)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

SOURCE=../../../../

export CXXFLAGS+=-O3
export CFLAGS+=-O3

ifeq ($(CROSS_COMPILING),yes)
  CONFIG_EXTRA += -DWITH_ARCH=$(HOST) -DWITH_CPU=$(CPU) -DENABLE_INTERNAL_FFMPEG=ON -DENABLE_INTERNAL_CROSSGUID=OFF
endif

ifeq ($(TARGET_PLATFORM),raspberry-pi)
  CONFIG_EXTRA += -DENABLE_PULSEAUDIO=OFF -DENABLE_X11=OFF -DENABLE_OPENGL=OFF -DENABLE_VAAPI=OFF -DENABLE_VDPAU=OFF -DENABLE_MMAL=ON
endif

ifeq ($(OS),android)
CONFIGURE += --enable-codec=libstagefright,amcodec --enable-breakpad
endif

ifeq ($(Configuration),Release)
DEBUG = --enable-debug=no
endif

CONFIGURE += $(CONFIG_EXTRA)

all: $(SOURCE)/lib$(APP_NAME).so

release: DEBUG=--enable-debug=no
release: $(SOURCE)/lib$(APP_NAME).so

debug: DEBUG=--enable-debug=yes
debug: $(SOURCE)/lib$(APP_NAME).so

$(SOURCE)/lib$(APP_NAME).so:
#	cd $(SOURCE); BOOTSTRAP_FROM_DEPENDS=yes ./bootstrap
	mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(CMAKE) -DCMAKE_INSTALL_PREFIX=$(PREFIX) $(CONFIG_EXTRA) -DVERBOSE=0 VERBOSE=1 $(XBMCROOT)/project/cmake
	cd $(PLATFORM); $(MAKE) -j4

../../Makefile.include:
	$(error Please run configure)

clean:
	cd $(SOURCE); $(MAKE) clean

distclean:
	cd $(SOURCE); $(MAKE) clean
