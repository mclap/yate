@echo off
echo ============== test1: strings
yate tests/10_strings.tpl
echo ============== test2: consts and vars
yate tests/11_consts.tpl
echo off
echo ============== test3: if
yate tests/13_if.tpl
echo off
echo ============== test4: while
yate tests/14_while.tpl | wc
echo off
echo ============== test5: include
yate tests/15_include.tpl