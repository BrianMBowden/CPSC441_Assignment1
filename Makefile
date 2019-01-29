##################################################################################################
# GENERIC MAKEFILE FOR C AND C++
##################################################################################################
# Author: Brian Bowden
# Date: September 23, 2018
##################################################################################################
# A generic makefile I came up with after reading some of the GNU "make" documentation
# https://www.gnu.org/software/make/manual/make.html
##################################################################################################

CC=g++
FLAGS=-Wall -pedantic -I. -fdiagnostics-color -g
HEADERS= $(wildcard *.h)
TARGET=myProxy
OBJECTS= $(RECIPE:.c=.o)
RECIPE= $(wildcard *.c)

%.o: %.c
	$(CC) -c $< $(FLAGS) -o $@

all: $(OBJECTS) $(HEADERS)
	$(CC) $(OBJECTS) -o $(TARGET)

clean:
	rm -f *.o *~ $(TARGET)

run:
	make all
	./$(TARGET)
