OUTPUT_PATH = ./bin
SRC_PATH = ./
OUTPUT_NAME = elprofiler.so

SRCS = buffer.c callstack.c clocks.c elprof_core.c elprofiler.c elprof_logdata.c elprof_logger.c file_buffer.c
OBJS = $(SRCS:%.c=%.o)

LUA = $(HOME)/running/3rd/lua/lua-5.1.3_coco-1.1.4/src

INCS = -I$(LUA) -I$(SRC_PATH)
CC = gcc
WARN = -W -Wall
CFLAGS = -O2 -fPIC $(WARN) $(INCS)

all: profiler 
all: tool

profiler: $(OBJS)
		mkdir -p $(OUTPUT_PATH) && $(LD) -shared -o $(OUTPUT_PATH)/$(OUTPUT_NAME) $(OBJS)

tool:
	-(cd analyzer && make OUTPUT_PATH=$(PWD)/$(OUTPUT_PATH))

debug: CFLAGS = -g -DDEBUG -fPIC $(WARN) $(INCS)
debug: profiler
	   -(cd analyzer && make debug OUTPUT_PATH=$(PWD)/$(OUTPUT_PATH))

clean:
		-(rm -f $(OUTPUT_PATH)/$(OUTPUT_NAME) $(SRC_PATH)/*.o)
		-(cd analyzer && make clean OUTPUT_PATH=$(OUTPUT_PATH))

test: OUTPUT_PATH = ./test
test: profiler tool
test:
	  -(cd test && make)

test_debug: OUTPUT_PATH = ./test
test_debug:	debug
test_debug:	
			-(cd test && make debug)
