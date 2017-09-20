yash : yash.o
				gcc yash.c -o yash
yash.o : yash.c
	      gcc -c yash.c
clean:
				rm *.o yash
