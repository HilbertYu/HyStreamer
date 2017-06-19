###############################################################################
# compiler setting
###############################################################################
CC       = gcc
CXX      = g++
CFLAGS   = -g -Wall
CXXFLAGS = $(CFLAGS) -Weffc++

LIBS     = -lm $(shell  pkg-config --libs /usr/local/Cellar/ffmpeg/3.3.2/lib/pkgconfig/lib*)
LIBS     += -lpthread $(shell pkg-config --libs /usr/local/Cellar/opencv3/3.2.0/lib/pkgconfig/opencv.pc)
LIBS     += -L/usr/local/lib -lSDL2

FFMPEG_INCPATH=/usr/local/Cellar/ffmpeg/3.3.2/include

INCPATH  += -I$(FFMPEG_INCPATH) \

INCPATH  += -I/usr/local/Cellar/opencv3/3.2.0/include/

INCPATH  += -I/usr/local/Cellar/sdl2/2.0.5/include/

DIR     = $(shell pwd)

###############################################################################
# source files setting
###############################################################################
C_SOURCES   = $(shell find . -name "*.c")
#CXX_SOURCES = $(shell find . -name "*.cpp")
CXX_SOURCES = main.cpp
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
