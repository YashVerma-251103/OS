all:
	make make_and_move
	make run
# make clean

#invoke make inside following directories and in this order: loader, launch, fib
#move the lib_simpleloader.so and launch binaries inside bin directory
make_and_move:
	make -C loader/
	mv loader/lib_simpleloader.so bin/lib_simpleloader.so
	cd launcher/ && make
	mv launcher/launch bin/launch
	cd test && make
	mv test/fib bin/fib
run:
	cd bin && ./launch fib
#Provide the command for cleanup
clean:
	-@rm -rf bin/*