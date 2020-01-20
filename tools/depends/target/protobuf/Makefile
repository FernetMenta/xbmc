-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=protobuf
VERSION=3.8.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL=https://github.com/protocolbuffers/protobuf
TAG=v3.8.0.tar.gz

CMAKE_OPTIONS=-Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_PROTOC_BINARIES=ON
CMAKE_OPTIONS_PROTOC= -Dprotobuf_BUILD_TESTS=OFF \
											-DCMAKE_TOOLCHAIN_FILE= \
											-DCMAKE_INSTALL_PREFIX="$(NATIVEPREFIX)" \
											-DCMAKE_C_COMPILER="$(CC_BINARY_FOR_BUILD)" \
											-DCMAKE_CXX_COMPILER="$(CXX_BINARY_FOR_BUILD)" \
											-DCMAKE_C_FLAGS="$(NATIVE_CFLAGS)" \
											-DCMAKE_CXX_FLAGS="$(NATIVE_CXXFLAGS)" \
											-DCMAKE_EXE_LINKER_FLAGS="$(NATIVE_LDFLAGS)"

PROTOC=$(NATIVEPREFIX)/bin/protoc

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/archive/$(TAG)
	cd $(TARBALLS_LOCATION); mv $(TAG) $(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build

$(PROTOC): $(PLATFORM)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS_PROTOC) ../cmake
	cd $(PLATFORM)/build; $(MAKE) install
	cd $(PLATFORM); rm -rf build; mkdir -p build

.installed-$(PLATFORM): $(PLATFORM) $(PROTOC)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ../cmake
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
