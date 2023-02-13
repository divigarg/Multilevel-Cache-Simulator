G++ := g++
INCLUDE := ./include/
SRC_DIR	:= ./src

SRCS := $(shell find $(SRC_DIR) -name "*.cpp" )

all: bin/simulator

bin/simulator:
	mkdir bin
	$(G++) -I $(INCLUDE) -o $@ $(SRCS)



clean:
	rm -rf bin
	echo '' >  logs/debug.log