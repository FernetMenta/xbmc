SRCS=

ifeq (1,1)
SRCS += RendererVAAPI.cpp
SRCS += RendererVDPAU.cpp
endif

ifeq (,1)
SRCS += MMALRenderer.cpp
endif
     
LIB=HwDecRender.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
