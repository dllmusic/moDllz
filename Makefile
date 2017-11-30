FLAGS += -I../../src/core

SOURCES = $(wildcard src/*.cpp)


include ../../plugin.mk


dist: all
	mkdir -p dist/moDllz
	cp LICENSE* dist/moDllz/
	cp $(TARGET) dist/moDllz/
	cp -R res dist/moDllz/
