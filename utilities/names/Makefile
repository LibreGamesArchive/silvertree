OBJECTS=namegen.o
LD_FLAGS=
CC_FLAGS=-O2 -c  -I/usr/local/include/boost-1_34
CC=g++
EXECUTABLE=namegen

all:	$(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE) *.o

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LD_FLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CC_FLAGS) $< -o $@
