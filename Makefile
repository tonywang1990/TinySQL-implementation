all: TestStorageManager

TestStorageManager: StorageManager.o 
	g++ -o TestStorageManager StorageManager.o TestStorageManager.cpp

StorageManager.o: Block.h Disk.h Field.h MainMemory.h Relation.h Schema.h SchemaManager.h Tuple.h Config.h
	g++ -c StorageManager.cpp

clean:
	rm *.o TestStorageManager
