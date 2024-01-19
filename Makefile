compile: ./mheap.c
	gcc -c ./mheap.c
	gcc ./mheap.o -o heap

run:
	./heap

clean:
	rm ./mheap.o ./heap