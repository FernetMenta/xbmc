-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=grpc
VERSION=1.26.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL=https://github.com/grpc/grpc
TAG=v1.26.0.tar.gz

CMAKE_OPTIONS=-DgRPC_INSTALL=ON -DgRPC_BUILD_CODEGEN=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_SSL_PROVIDER=package
CMAKE_OPTIONS_PLUGIN=	-DCMAKE_TOOLCHAIN_FILE= \
											-DgRPC_BUILD_TESTS=OFF \
											-DgRPC_PROTOBUF_PROVIDER=package \
											-DgRPC_ZLIB_PROVIDER=package \
											-DgRPC_CARES_PROVIDER=package \
											-DgRPC_SSL_PROVIDER=module \
											-DgRPC_BUILD_CODEGEN=ON \
 											-Dprotobuf_BUILD_TESTS=OFF \
											-DCMAKE_INSTALL_PREFIX="$(NATIVEPREFIX)" \
											-DCMAKE_C_COMPILER="$(CC_BINARY_FOR_BUILD)" \
											-DCMAKE_CXX_COMPILER="$(CXX_BINARY_FOR_BUILD)" \
											-DCMAKE_C_FLAGS="$(NATIVE_CFLAGS)" \
											-DCMAKE_CXX_FLAGS="$(NATIVE_CXXFLAGS)" \
											-DCMAKE_EXE_LINKER_FLAGS="$(NATIVE_LDFLAGS)"

PROTOC_PLUGIN=$(NATIVEPREFIX)/bin/grpc_cpp_plugin

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
	cd $(PLATFORM); rm -rf cmake/build; mkdir -p cmake/build

$(PROTOC_PLUGIN): $(PLATFORM)
	cd $(PLATFORM)/cmake/build; $(CMAKE) $(CMAKE_OPTIONS_PLUGIN) ../..
	cd $(PLATFORM)/cmake/build; $(MAKE) grpc_cpp_plugin
	mv $(PLATFORM)/cmake/build/grpc_cpp_plugin $(NATIVEPREFIX)/bin
	cd $(PLATFORM); rm -rf cmake/build; mkdir -p cmake/build

.installed-$(PLATFORM): $(PLATFORM) $(PROTOC_PLUGIN)
	cd $(PLATFORM)/cmake/build; $(CMAKE) $(CMAKE_OPTIONS) ../..
	cd $(PLATFORM)/cmake/build; $(MAKE) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/cmake/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
