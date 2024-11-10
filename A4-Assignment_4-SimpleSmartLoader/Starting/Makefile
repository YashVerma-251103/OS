all:
	make compile
	make run
	make clean

compile:
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -no-pie -nostdlib -o sum sum.c
	gcc -m32 -o loader loader.c

run:
	./loader ./fib
	./loader ./sum

clean:
	-@rm -f fib loader sum
