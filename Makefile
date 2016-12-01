TARGET=Tiny

CXX=g++ -g

FLAGS= -Wall 

LIB=


HDRS= utility.h Tiny.h query.h lqp.h

SRCS= utility.cpp Tiny.cpp query.cpp lqp.cpp

OBJS= utility.o Tiny.o query.o lqp.o



$(TARGET):  $(OBJS) StorageManager.o 
	$(CXX) $(FLAGS) $(OBJS) StorageManager.o $(LIB) -o $(TARGET) 

$(OBJS): $(HDRS)

StorageManager.o: Block.h Disk.h Field.h MainMemory.h Relation.h Schema.h SchemaManager.h Tuple.h Config.h
	g++ -c StorageManager.cpp



clean:
	rm $(OBJS) $(TARGET)
