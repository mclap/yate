: flex tpl.l
: bison -d tpl.y
D:\Apps\mingw\bin\flex.exe -i -otpl_scanner.c tpl_scanner.l
bison -v -d tpl_parser.y -o tpl_parser.c
gcc -g -o yate tpl_parser.c tpl_scanner.c yatefunc.c main_cli.c
