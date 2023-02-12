G++ := g++
INCLUDE := ./include/
SRC_DIR	:= ./src

SRCS := $(shell find $(SRC_DIR) -name "*.cpp" )

all: bin/simulator

bin/simulator:
	$(G++) -I $(INCLUDE) -o $@ $(SRCS)



clean:
	rm -f bin/simulator
	echo '' >  logs/debug.log