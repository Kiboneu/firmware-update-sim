NANOPB_DIR  = ./nanopb
NANOPB_CORE = $(NANOPB_DIR)/pb_encode.c $(NANOPB_DIR)/pb_decode.c $(NANOPB_DIR)/pb_common.c
PROTOC      = $(NANOPB_DIR)/generator/protoc

CC = g++
CPPFLAGS    = -Wall -Werror -std=c++11 -ggdb
CPPFLAGS   += "-I$(NANOPB_DIR)" -pthread

SRC         = ./server.cpp ./client.cpp
SRC        += ./crc_lib.cpp ./nanopb_wrapper.cpp
SRC        += ./fw_update.pb.c
SRC        += $(NANOPB_CORE)

all: clean build-pb build

build:
	$(CC) $(CPPFLAGS) $(SRC) -o fw_update_sim

build-pb:
	$(PROTOC) --nanopb_out=. fw_update.proto

clean:
	rm -f fw_update.pb.c fw_update.pb.h

debug: all
	gdb ./fw_update_sim