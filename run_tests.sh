echo "============== test1: strings"
./yate tests/10_strings.tpl
echo "============== test2: consts & vars"
./yate tests/11_consts.tpl
echo "============== test3: if"
./yate tests/13_if.tpl
echo "============== test4: while"
./yate tests/14_while.tpl | wc
echo "============== test5: include"
