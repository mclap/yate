: flex tpl.l
: bison -d tpl.y
D:\Apps\mingw\bin\flex.exe -i -otpl_scanner.c tpl_scanner.l
bison -v -d tpl_parser.y -o tpl_parser.c
gcc -g -L"D:/Apps/DevCpp/lib" -I"D:/Apps/DevCpp/include" -I"include" -L"lib" tpl_parser.c tpl_scanner.c lextpl.c
: -lstrfunc -lpcreposix -lpcre 
