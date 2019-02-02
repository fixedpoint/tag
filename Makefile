.PHONY: all check clean

all: tag

check:
	./tag -h > /dev/null
	./tag --help > /dev/null
	./tag --version > /dev/null
	./tag test/rule0 aaa
	./tag test/rule1 abbba
	./tag -i test/rule2 aaa

clean:
	-rm -f tag

tag: tag.c
	$(CC) -W -Wall -o $@ $<
