/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
Copyright (c) 2019 Takeshi Abe <tabe@fixedpoint.jp>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_VERSION "0.9.0"

#define TAG_STATUS_SUCCESS 0
#define TAG_STATUS_CYCLE 1
#define TAG_STATUS_ERROR 2

#define TAG_DEFAULT_MAX_LENGTH 100
#define TAG_DEFAULT_NUM_STEPS 100

#define TAG_HELP "tag [OPTIONS]... [FILE] [INPUT]\n"					\
	"Options:\n"														\
	"  -d NUM\n"														\
	"     specify the deletion number (default 2)\n"					\
	"  -i, --indent\n"													\
	"     indent output, the depth of indentation is the deletion number multiplied by the step number\n" \
	"  -m, -m LENGTH\n"													\
	"     exit if the length of the current word exceeds LENGTH (default 100)\n" \
	"  -n, -n NUM\n"													\
	"     exit if the number of steps reaches NUM (default 100)\n"		\
	"  -q, --quiet\n"													\
	"     suppress output\n"											\
	"  -h, --help\n"													\
	"     display this help and exit\n"									\
	"  --version\n"														\
	"     output version information and exit\n"

static int num_deletion = 2;

static int indent = 0;

static int max_length = 0;

static int num_steps = 0;

static int quiet = 0;

struct rule {
	int head;
	char *tail;
	size_t len_tail;
};

static int num_rules = 0;

static struct rule *rules = NULL;

static FILE *fp;

static int num_lines = 0;

static void free_rules(void)
{
	for (int i=0;i<num_rules;i++)
		free(rules[i].tail);
	free(rules);
}

static int skip_line(void)
{
	int c = fgetc(fp);
	while (c != EOF) {
		switch (c) {
		case '\r':
			++num_lines;
			c = fgetc(fp);
			if (c == '\n')
				return fgetc(fp);
			return c;
		case '\n':
			++num_lines;
			return fgetc(fp);
		default:
			c = fgetc(fp);
			break;
		}
	}
	return EOF;
}

static void memory_error_at_compilation(void)
{
	free_rules();
	fclose(fp);
	fprintf(stderr, "line %d: a memory error occurred\n", num_lines + 1);
	exit(TAG_STATUS_ERROR);
}

static int read_tail(void)
{
	int c = fgetc(fp);
	if (c == EOF) {
		free_rules();
		fclose(fp);
		fprintf(stderr, "line %d: missing tail of rule\n", num_lines + 1);
		exit(TAG_STATUS_ERROR);
	}
	if (c != ' ') {
		free_rules();
		fclose(fp);
		fprintf(stderr, "line %d: no space follows head\n", num_lines + 1);
		exit(TAG_STATUS_ERROR);
	}
	while ( (c = fgetc(fp)) == ' ' ) ;
	size_t len_tail;
	char *tail;
	if (c == EOF) {
		len_tail = 0;
		tail = NULL;
	} else if (c == '\r') {
		c = fgetc(fp);
		if (c == '\n')
			c = fgetc(fp);
		len_tail = 0;
		tail = NULL;
	} else if (c == '\n') {
		len_tail = 0;
		tail = NULL;
	} else {
		len_tail = 1;
		tail = malloc(len_tail);
		if (!tail)
			memory_error_at_compilation();
		tail[0] = (char)c;
		while ( (c = fgetc(fp)) != EOF) {
			if (c == '\r') {
				c = fgetc(fp);
				if (c == '\n')
					c = fgetc(fp);
				break;
			}
			if (c == '\n') {
				c = fgetc(fp);
				break;
			}
			if (c == ' ') {
				free_rules();
				fclose(fp);
				fprintf(stderr, "line %d: space in tail\n", num_lines + 1);
				exit(TAG_STATUS_ERROR);
			}
			char *t = realloc(tail, len_tail + 1);
			if (!t) {
				free(tail);
				memory_error_at_compilation();
			}
			t[len_tail++] = (char)c;
			tail = t;
		}
	}
	rules[num_rules].tail = tail;
	rules[num_rules].len_tail = len_tail;
	++num_rules;
	++num_lines;
	return c;
}

static void compile(const char *file)
{
	fp = fopen(file, "rb");
	if (!fp) {
		fprintf(stderr, "failed to open %s\n", file);
		exit(TAG_STATUS_ERROR);
	}
	int c = fgetc(fp);
	struct rule *r;
	while (c != EOF) {
		switch (c) {
		case '\r':
			++num_lines;
			c = fgetc(fp);
			if (c == '\n')
				c = fgetc(fp);
			break;
		case '\n':
			++num_lines;
			c = fgetc(fp);
			break;
		case ' ':
			c = skip_line();
			break;
		default:
			for (int k=0;k<num_rules;k++) {
				if (rules[k].head == c) {
					free_rules();
					fclose(fp);
					fprintf(stderr, "line %d: more than one rules have head %c\n",
							num_lines + 1, c);
					exit(TAG_STATUS_ERROR);
				}
			}
			r = realloc(rules, sizeof(*rules) * (num_rules + 1));
			if (!r)
				memory_error_at_compilation();
			r[num_rules].head = c;
			r[num_rules].tail = NULL;
			rules = r;
			c = read_tail();
			break;
		}
	}
	fclose(fp);
}

static int run(const char *input)
{
	if (strchr(input, ' ')) {
		free_rules();
		fprintf(stderr, "space in input: %s\n", input);
		return TAG_STATUS_ERROR;
	}

	if (!quiet)
		puts(input);

	size_t len = strlen(input);
	if (len < (size_t)num_deletion) {
		free_rules();
		return TAG_STATUS_SUCCESS;
	}

	size_t len_buf = len * 2;
	char *buf = malloc(len_buf);
	if (!buf) {
		free_rules();
		fprintf(stderr, "a memory error occurred\n");
		return TAG_STATUS_ERROR;
	}
	strcpy(buf, input);

	int n = 1;
 step:
	if (len < (size_t)num_deletion)
		goto done;
	if (num_steps > 0 && num_steps <= n) {
		free(buf);
		free_rules();
		return TAG_STATUS_CYCLE;
	}
	if (max_length > 0 && (size_t)max_length < len) {
		free(buf);
		free_rules();
		return TAG_STATUS_CYCLE;
	}
	for (int k=0;k<num_rules;k++) {
		if (rules[k].head == buf[0]) {
			if (len + rules[k].len_tail <= (size_t)num_deletion)
				goto done;
			size_t cap = len + rules[k].len_tail - num_deletion;
			if (len_buf < cap) {
				char *b = realloc(buf, cap);
				if (!b) {
					free(buf);
					free_rules();
					fprintf(stderr, "a memory error occurred\n");
					return TAG_STATUS_ERROR;
				}
				buf = b;
				len_buf = cap;
			}
			if (len < (size_t)num_deletion) {
				size_t len1 = num_deletion - len;
				len = rules[k].len_tail - len1;
				memcpy(buf, rules[k].tail + len1, len);
			} else if (len == (size_t)num_deletion) {
				if (0 < rules[k].len_tail)
					memcpy(buf, rules[k].tail, rules[k].len_tail);
				len = rules[k].len_tail;
			} else {
				len -= num_deletion;
				memmove(buf, buf + num_deletion, len);
				memcpy(buf + len, rules[k].tail, rules[k].len_tail);
				len += rules[k].len_tail;
			}
			if (!quiet) {
				if (indent)
					for (int i=0;i<n*num_deletion;i++)
						putchar(' ');
				fwrite(buf, len, 1, stdout);
				putchar('\n');
			}
			++n;
			goto step;
		}
	}
 done:
	free(buf);
	free_rules();
	return TAG_STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
	if (argc >= 2) {
		for (int i=0;i<argc;i++) {
			if ( strcmp(argv[i], "-h") == 0 ||
				 strcmp(argv[i], "--help") == 0 ) {
				fputs(TAG_HELP, stdout);
				return TAG_STATUS_SUCCESS;
			}
			if (strcmp(argv[i], "--version") == 0) {
				puts("tag " TAG_VERSION);
				return TAG_STATUS_SUCCESS;
			}
		}
	}
	if (argc < 2) {
		fputs(TAG_HELP, stderr);
		return TAG_STATUS_ERROR;
	}
	int i = 1;
	while (i < argc - 2) {
		if (strcmp(argv[i], "-d") == 0) {
			int n = atoi(argv[++i]);
			if (n > 0) {
				num_deletion = n;
				++i;
			} else {
				fputs(TAG_HELP, stderr);
				return TAG_STATUS_ERROR;
			}
			continue;
		}
		if ( strcmp(argv[i], "-i") == 0 ||
			 strcmp(argv[i], "--indent") == 0 ) {
			indent = 1;
			++i;
			continue;
		}
		if (strcmp(argv[i], "-m") == 0) {
			int m = atoi(argv[++i]);
			if (m > 0) {
				max_length = m;
				++i;
			} else {
				max_length = TAG_DEFAULT_MAX_LENGTH;
			}
			continue;
		}
		if (strcmp(argv[i], "-n") == 0) {
			int n = atoi(argv[++i]);
			if (n > 0) {
				num_steps = n;
				++i;
			} else {
				num_steps = TAG_DEFAULT_NUM_STEPS;
			}
			continue;
		}
		if ( strcmp(argv[i], "-q") == 0 ||
			 strcmp(argv[i], "--quiet") == 0 ) {
			quiet = 1;
			++i;
			continue;
		}
		break;
	}
	if (argc - 2 != i) {
		fputs(TAG_HELP, stderr);
		return TAG_STATUS_ERROR;
	}
	compile(argv[i]);
	return run(argv[++i]);
}
