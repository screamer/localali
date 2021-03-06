OS = $(shell uname -s)
CXX = g++
DEBUG = yes

# Default mode is "Debug"
DEFAULT_MODE  = Debug
MODE         ?= $(DEFAULT_MODE)

# If mode is something other than "Debug" or "Release",throw a fit
ifneq ($(MODE),Debug)
ifneq ($(MODE),Release)
$(error MODE must be one of {Debug,Release})
endif
endif

LEMON = include/lemon-1.2.3

ifeq ($(MODE),Debug)
	CXXFLAGS = -Wall -g3 -DDEBUG -fopenmp -std=c++0x -DVERBOSE -I$(LEMON)/ -Isrc/
else
	CXXFLAGS = -Wall -O3 -ffast-math -fcaller-saves -finline-functions -fopenmp -std=c++0x -DNDEBUG -I$(LEMON)/ -Isrc/
endif

all: localalitmp move

localalitmp: src/main.cpp src/verbose.o $(LEMON)/lemon/arg_parser.o
	${CXX} ${CXXFLAGS} -o $@ $^ 

move:
	mv localalitmp localali

#lemon: lemon-config lemon-make

#lemon-config:
#$(LEMON)/configure

#lemon-make:
#$(LEMON)/make	

clean:
	rm localali
