COMP362-P1
==========

FUSE Filesystem

Compile with `$ gcc src/*.c $(pkg-config --cflags --libs glib-2.0) $(pkg-config fuse --cflags --libs) -lpthread -g -o bin/p1t4 -g`.

Run with `./p1t4 -s -d ./tmp` for verbal. Or `./p1t4 ./tmp` for silent.

Run valgrind with `G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --leak-check=yes ./p1t4 -s -d ./tmp`.

