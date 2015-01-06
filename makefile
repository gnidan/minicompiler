.PHONY: run

compiler: compilerext.tab.cpp lex.yy.o
	g++ compilerext.tab.cpp programext.cpp ralprogram.cpp lex.yy.o -o compiler

run: compiler
	./compiler

compilerext.tab.cpp:
	bison compilerext.ypp

lex.yy.o: lex.yy.c
	gcc -c lex.yy.c

lex.yy.c: programext.tab.h
	flex programext.l

programext.tab.h:
	bison -d programext.y

clean:
	rm *.tab.*
	rm compiler
	rm lex.yy.*
