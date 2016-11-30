TARGET=Tiny

CXX=g++

FLAGS=

LIB=


HDRS= utility.h

SRCS= utility.cpp Tiny.cpp

OBJS= utility.o Tiny.o



$(TARGET):  $(OBJS) StorageManager.o 
	$(CXX) $(FLAGS) $(OBJS) StorageManager.o $(LIB) -o $(TARGET) 

$(OBJS): $(HDRS)

StorageManager.o: Block.h Disk.h Field.h MainMemory.h Relation.h Schema.h SchemaManager.h Tuple.h Config.h
	g++ -c StorageManager.cpp



clean:
	rm $(OBJS) $(TARGET)
