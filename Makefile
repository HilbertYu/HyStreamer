###############################################################################
# compiler setting
###############################################################################
CC       = gcc
CXX      = g++
CFLAGS   = -g -Wall
CXXFLAGS = $(CFLAGS) -Weffc++
LIBS     = -lm $(shell  pkg-config --libs /usr/local/Cellar/ffmpeg/3.3.2/lib/pkgconfig/lib*)

FFMPEG_INCPATH=/usr/local/Cellar/ffmpeg/3.3.2/include

INCPATH  += -I$(FFMPEG_INCPATH) \
			-I$(FFMPEG_INCPATH)/libavcodec \
			-I$(FFMPEG_INCPATH)/libavutil \
			-I$(FFMPEG_INCPATH)/libavformat \
			-I$(FFMPEG_INCPATH)/libswscale \

DIR     = $(shell pwd)

###############################################################################
# source files setting
###############################################################################
C_SOURCES   = $(shell find . -name "*.c")
CXX_SOURCES = $(shell find . -name "*.cpp")
C_OBJS      = $(patsubst %.c,%.o,$(wildcard $(C_SOURCES)))
CXX_OBJS    = $(patsubst %.cpp,%.o,$(wildcard $(CXX_SOURCES)))
OBJS        = $(C_OBJS) $(CXX_OBJS)
EXEC      = $(shell basename $(DIR))

###############################################################################
.PHONY : clean clean_all
###############################################################################
all: $(EXEC)

%.o:%.c
	$(CC) -c $(CFLAGS) $(INCPATH) $< -o $@
%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $< -o $@

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC) $(LIBS)

###############################################################################
clean:
	@rm -vfr $(OBJS)
clean_all: clean
	@rm -vfr $(EXEC)
###############################################################################
