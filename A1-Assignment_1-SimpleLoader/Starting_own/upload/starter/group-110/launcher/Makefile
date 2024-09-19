# Compile the launch.c by linking it with the lib_simpleloader.so
compile: launch.c ../bin/lib_simpleloader.so
	gcc -m32 -L../bin -I../loader -Wl,-rpath=../bin  -o launch launch.c -l_simpleloader
	
#Provide the command for cleanup
# clean:
# already covered in the main Makefile.