# Makefile

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -g3

# Paths
INCLUDES = -Ic:/Users/melle/Desktop/sdlvoorjari/project/include
LIBDIR = -Lc:/Users/melle/Desktop/sdlvoorjari/project/lib

# Source files
SRC = c:/Users/melle/Desktop/sdlvoorjari/main.cpp

# Output
OUT = c:/Users/melle/Desktop/sdlvoorjari/output/main.exe

# Libraries
LIBS = c:/Users/melle/Desktop/sdlvoorjari/project/lib/libSDL3.dll.a

# Target
all:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBDIR) $(SRC) -o $(OUT) $(LIBS)
