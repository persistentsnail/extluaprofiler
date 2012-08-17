CXX = g++
INCS = -I../ -I./
CPPFLAGS = -std=c++0x $(INCS)

SRCS = analyzer.cc main.cc
OBJS = $(SRCS:%.cc=%.o)

ifndef $(OUTPUT_PATH)
	OUTPUT_PATH = ./
endif

analyzer: $(OBJS)
	$(CXX) -o $(OUTPUT_PATH)/analyzer $(OBJS)

debug: CPPFLAGS += -g -DDEBUG 
debug: analyzer


clean:
	rm -f *.o $(OUTPUT_PATH)/analyzer
