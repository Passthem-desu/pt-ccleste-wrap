let os = (sys host | get name)
let target = if $os == "Windows" { "ccleste-wrap.exe" } else { "ccleste-wrap" }

print $"正在为($os)构建，目标为 ./dist/($target)"

mkdir build
mkdir dist

g++ -DCELESTE_P8_FIXEDP -c ./libs/ccleste/celeste.c -o ./build/celeste.o
go build -o $"./dist/($target)"
