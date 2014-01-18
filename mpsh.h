/*

Copyright (c) 2013, Dave Fischer 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions 
are met:

* Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer 
	in the documentation and/or other materials provided with 
	the distribution.

* Neither the name of the The Center for Computational Aesthetics nor 
	the names of its contributors may be used to endorse or promote
	products derived from this software without specific prior 
	written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------

mpsh by Dave Fischer http://www.cca.org

mpsh.h

	global structs and defines

*/




#include <unistd.h>



#define FLAG_LOWER		0xFFFF
#define FLAG_REDIRECT	0x0001
#define FLAG_BACK		0x0002
#define FLAG_BATCH		0x0004
#define FLAG_SMP		0x0008
#define FLAG_PIPE		0x0010
#define FLAG_COND		0x0020
#define FLAG_NEGCOND	0x0040
#define FLAG_NICE		0x0080
#define FLAG_SMP_SYM	0x0100
#define FLAG_GROUP		0x0200
#define FLAG_NOTERM		0x0400

#define FLAG_SETOP				0xFF0000
#define FLAG_SETOP_UNION		0x010000
#define FLAG_SETOP_INTER		0x020000
#define FLAG_SETOP_DIFF			0x040000
#define FLAG_SETOP_SYMM			0x080000
#define FLAG_SETOP_SUBSET		0x100000
#define FLAG_SETOP_SUBSET_REV	0x200000
#define FLAG_SETOP_EQUALS		0x400000



#define IO_OUT			0x0001
#define IO_ERR			0x0002
#define IO_OUT_APPEND	0x0004
#define IO_ERR_APPEND	0x0008


#define QUOTE_STACK_DEPTH 32
#define QUOTE_DOUBLE	0x01
#define QUOTE_SINGLE	0x02
#define QUOTE_PAREN		0x03
#define QUOTE_BRACE		0x04
#define QUOTE_SQUARE	0x05

/* Number of bytes to allocate at a time for words. */
#define WORD_ALLOC 32 

struct word_list {
	int space; /* Bytes malloc'ed. */
	char *word;
	struct word_list *next;
} ;

struct command {
	char *text;		/* command text for display, history */
	char *expansion; /* command text, fully parsed */
	char *dir;		/* cwd before command executes, for history */
	char *echo_text;	/* for history field display */
	struct word_list *words;	/* command & args */
	struct word_list *handler_args;
	int flags;
	int nice;
	int pipe_io_flags;
	int file_io_flags;
	int smp_num;		/* number of processes to generate */
	int smp_id;			/* id so they all find the same history entry */
	char display_text;	/* boolean: display history sub notice? */
	char job_handler;
	pid_t pid;
	struct word_list *stderr_filename;
	struct word_list *stdout_filename;
	struct command *pipeline;	/* next command in sequence */
	/* Parsing */
	char state;
	char ch;
	char backslash;
	int start_of_sub;
	int quote_depth;
	char quote_stack[QUOTE_STACK_DEPTH];
} ;

extern struct word_list *global_env;
extern struct word_list *search_path;

struct command *parse_char();
struct command *parse_char_limited();
struct word_list *init_word();
struct word_list *init_word_str();
struct word_list *find_last_word();
struct word_list *find_next_to_last_word();
struct word_list *new_word();
struct word_list *new_arg_word();
struct command *init_command();
struct command *expand_history();
struct command *parse_string();
struct command *insert_parse_string();


char *dup_history_dir();




char *get_env();
char *words_to_string();
char *find_path();

char *read_from_command();
char *get_prompt();
struct word_list *words_from_command();
char *get_command_expansion();


char *end_of_string();
char *dup_cwd();

char *find_alias();


extern int control_term;



/* For builtin commands: */
#define PARENT  0
#define CHILD   1

/* For parse_and_run() */
#define INTERACTIVE 1
#define NONINTERACTIVE 0

#define NORMAL_JOB 0
#define GROUP_JOB 1


int parse_depth;
#define MAX_PARSE_DEPTH 32


