OBJECTS = testcase.o buf.o Disk.o

OS2 : $(OBJECTS)
	gcc -o OS2 $(OBJECTS)

testcase.o : testcase.c queue.h buf.h Disk.h
	gcc -c testcase.c

buf.o : buf.c buf.h queue.h Disk.h
	gcc -c buf.c

Disk.o : Disk.c Disk.h
	gcc -c Disk.c
