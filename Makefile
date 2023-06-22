.PHONY: all check clean install

all: tag

check:
	./tag -h > /dev/null
	./tag --help > /dev/null
	./tag --version > /dev/null
	./tag test/rule0 aaa | diff - test/output/rule0-aaa.txt
	./tag test/rule1 abbba | diff - test/output/rule1-abbba.txt
	./tag -i example/collatz.txt aaa | diff - test/output/collatz-aaa.txt
	./tag -e example/parity.txt a | diff - test/output/parity-a.txt
	./tag example/parity.txt aa | diff - test/output/parity-aa.txt
	./tag example/parity.txt aaa | diff - test/output/parity-aaa.txt

clean:
	-rm -f tag

install:
	install -d $(out)/bin
	install tag $(out)/bin/

tag: tag.c
	$(CC) -std=c11 -W -Wall -o $@ $<
