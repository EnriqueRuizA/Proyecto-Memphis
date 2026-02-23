# Medieval Conquest — Makefile (ajusta RAYLIB_PATH a tu instalación)
CXX      = g++
CXXFLAGS = -std=c++17 -Iinclude -O2 -Wall
RAYLIB_PATH ?= c:/raylib/raylib/src
LDFLAGS  = -L$(RAYLIB_PATH) -lraylib -lopengl32 -lgdi32 -lwinmm -lm

SRCS   = src/globals.cpp rts_game.cpp
TARGET = rts_game.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -I$(RAYLIB_PATH) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
