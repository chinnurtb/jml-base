include arch/cuda.mk

CAL_PATH := /usr/local/amdcal/
CAL_INCLUDE_PATH := /usr/local/amdcal/include

CXX := colorccache g++
CXXFLAGS := -I. -pipe -Wall -Werror -Wno-sign-compare -Woverloaded-virtual -O3 -fPIC -m64 -g -I/usr/include/eigen2 -I$(CUDA_INCLUDE_PATH) -I$(CAL_INCLUDE_PATH)
CXXLINKFLAGS := -shared -L$(BIN) -L$(CUDA_LIBRARY_PATH) -Wl,--rpath,$(BIN),--rpath,$(CUDA_LIBRARY_PATH)
CXXEXEFLAGS :=	-ltcmalloc -L$(BIN) -L$(CUDA_LIBRARY_PATH) -Wl,--rpath,$(BIN),--rpath,$(CUDA_LIBRARY_PATH)

FC := colorccache gfortran
FFLAGS := -I. -fPIC
