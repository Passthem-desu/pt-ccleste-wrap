mkdir build
mkdir dist

g++ -DCELESTE_P8_FIXEDP -c ./libs/ccleste/celeste.c -o ./build/celeste.o
go build -o ./dist/ccleste-wrap.exe
