fuse-proj
==========

FUSE Filesystem

Compile with `$ gcc src/*.c $(pkg-config --cflags --libs glib-2.0) $(pkg-config fuse --cflags --libs) -g -o bin/p1t4.out`.

Run with `./bin/p1t4.out -s -d ./tmp` for verbal. Or `./bin/p1t4.out ./tmp` for silent.

Run valgrind with `G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --leak-check=yes ./bin/p1t4.out -s -d ./tmp`.

