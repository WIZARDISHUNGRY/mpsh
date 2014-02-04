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

parse.c:

	parsing

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"
#include "state.h"


int unique_smp_id;


init_smp_id() {
	unique_smp_id = 100;
}

struct command *parse_char(command,ch)
struct command *command;
int ch;
{
	struct word_list *w, *ret_words;
	struct command *next_command;
	struct word_list *handler_args;
	char *alias_text;

	command->ch = ch;

	switch(command->state) {

		case STATE_WORD: /* Currently parsing a word */

			if(command->backslash) {
				command->backslash = 0;
				w = find_last_word(&command->words);
				add_letter_to_word(w,ch);
				return(command);
			}
			if(ch == '\\') {
				command->backslash = 1;
				return(command);
			}

			if(ch == '\"') {
				if(command->quote_stack[command->quote_depth] == QUOTE_DOUBLE) {
					command->quote_depth--;
					if(command->quote_depth < 1)
						return(command);
				} else {
					command->quote_stack[++command->quote_depth] =
						QUOTE_DOUBLE;
					if(command->quote_depth < 2)
						return(command);
				}
			}

			if(ch == '\'') {
				if(command->quote_stack[command->quote_depth] == QUOTE_SINGLE) {
					command->quote_depth--;
					if(command->quote_depth < 1)
						return(command);
				} else {
					command->quote_stack[++command->quote_depth] =
						QUOTE_SINGLE;
					if(command->quote_depth < 2)
						return(command);
				}
			}

			if(ch == '(') {
				command->quote_stack[++command->quote_depth] = QUOTE_PAREN;
				if(command->quote_depth == 1) {
					w = find_last_word(&command->words);
					command->start_of_sub = strlen(w->word);
				}
			}

			if(ch == ')') {
				if(command->quote_depth < 1 ||
					command->quote_stack[command->quote_depth] != QUOTE_PAREN) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->quote_depth--;
				if(command->quote_depth == 0) {
					char *str_to_exec;
					str_to_exec = find_last_word(&command->words)->word +
						command->start_of_sub;
					ret_words = words_from_command(str_to_exec+1);
					str_to_exec[0] = '\0';
					if(!parse_word_list(command,ret_words))
						return(NULL);
					return(command);
				}
			}

			if(ch == '[') {
				command->quote_stack[++command->quote_depth] = QUOTE_SQUARE;
			}

			if(ch == ']') {
				if(command->quote_depth < 1 ||
				command->quote_stack[command->quote_depth] != QUOTE_SQUARE) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->quote_depth--;
			}


			if(ch == '\n') {
				if(command->quote_depth > 0) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->state = STATE_DONE;
				command->quote_depth = 0;
				if(command->words->next == NULL) {
					if(command->words->word[0] == '!')
						command = expand_history(command);
					else {
						alias_text = find_alias(command->words->word);
						if(alias_text) {
							char *orig_str;
							orig_str = strdup(command->words->word);
							command = parse_string(command,alias_text);
							reset_command_text(command,orig_str);
							free(orig_str);
						}
					}
				}
				return(command);
			}
			if((ch == ' ' || ch == '\t') && command->quote_depth == 0) {
				command->state = STATE_SPACE;
				if(command->words->next == NULL) {
					if(command->words->word[0] == '!')
						command = expand_history(command);
					else {
						alias_text = find_alias(command->words->word);
						if(alias_text) {
							char *orig_str;
							orig_str = strdup(command->words->word);
							command = parse_string(command,alias_text);
							reset_command_text(command,orig_str);
							free(orig_str);
						}
					}
				}
				return(command);
			}
			w = find_last_word(&command->words);
			add_letter_to_word(w,ch);
			return(command);

		case STATE_COMMENT :

			if(ch == '\n') {
				command->state = STATE_DONE;
				return(command);
			}

			return(command);

		case STATE_SPACE :

			if(ch == '#') {
				command->state = STATE_COMMENT;
				return(command);
			}

			if(ch == '|') {
				command->state = STATE_PIPE;
				command->flags |= FLAG_PIPE;
				command->pipe_io_flags |= IO_OUT;
				return(command);
			}

			if(ch == '>') {
				command->state = STATE_REDIRECT;
				command->flags |= FLAG_REDIRECT;
				command->file_io_flags |= IO_OUT;
				return(command);
			}

			if(ch == ';') {
				command->state = STATE_SEQ;
				return(command);
			}

			if(ch == '&') {
				command->state = STATE_BACK;
				command->flags |= FLAG_BACK;
				return(command);
			}

			if(ch == '\n') {
				command->state = STATE_DONE;
				return(command);
			}

			if(ch == '\"') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_DOUBLE;
				w = new_arg_word(command);
				w->word[0] = 0;
				return(command);
			}

			if(ch == '\'') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_SINGLE;
				w = new_arg_word(command);
				w->word[0] = 0;
				return(command);
			}

			if(ch == '(') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_PAREN;
				command->start_of_sub = 0;
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

			if(ch == '[') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_SQUARE;
				if(!command->words)
					command->flags |= FLAG_GROUP;
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

			if(ch == ' ' || ch == '\t') {
				return(command);
			} else { 
				/* In a new word. */
				command->state = STATE_WORD;
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

		case STATE_PIPE :
			if(ch == ' ' || ch == '\t' || ch == '\n') {
				next_command = init_command();
				next_command->text = command->text;
				command->pipeline = next_command;
				command->state = STATE_SPACE;
				return(next_command);
			}
			if(ch == 'e') {
				command->pipe_io_flags |= IO_ERR;
				command->pipe_io_flags ^= IO_OUT;
				return(command);
			}
			if(ch == 'b') {
				command->pipe_io_flags |= IO_ERR;
				return(command);
			}
			if(ch == 'U' || ch == 'u') {
				command->flags |= FLAG_SETOP_UNION;
				return(command);
			}
			if(ch == '^') {
				command->flags |= FLAG_SETOP_INTER;
				return(command);
			}
			if(ch == '\\' || ch == '-') {
				command->flags |= FLAG_SETOP_DIFF;
				return(command);
			}
			if(ch == 'O' || ch == 'o') {
				command->flags |= FLAG_SETOP_SYMM;
				return(command);
			}
			if(ch == '<') {
				command->flags |= FLAG_SETOP_SUBSET;
				return(command);
			}
			if(ch == '>') {
				command->flags |= FLAG_SETOP_SUBSET_REV;
				return(command);
			}
			if(ch == '=') {
				command->flags |= FLAG_SETOP_EQUALS;
				return(command);
			}
			/* ERR */
			report_error("Unknown pipe option",NULL,ch,0);
			return(NULL);


		case STATE_REDIRECT :
			if(ch == ' ' || ch == '\t') {
				command->state = STATE_REDIRECT_SPACE;
				return(command);
			}
			if(ch == 'e') {
				command->file_io_flags |= IO_ERR;
				command->file_io_flags ^= IO_OUT;
				return(command);
			}
			if(ch == 'b') {
				command->file_io_flags |= IO_ERR;
				return(command);
			}
			if(ch == 'a') {
				if(command->file_io_flags & IO_ERR)
					command->file_io_flags |= IO_ERR_APPEND;
				else
					command->file_io_flags |= IO_OUT_APPEND;
				return(command);
			}
			/* ERR */
			report_error("Unknown IO option",NULL,ch,0);
			return(NULL);

		case STATE_REDIRECT_SPACE :
			if(ch == ' ' || ch == '\t') {
				return(command);
			} else {
				command->state = STATE_REDIRECT_FILE;
				if(command->file_io_flags & IO_ERR) {
					command->stderr_filename = init_word();
					add_letter_to_word(command->stderr_filename,ch);
				}
				if(command->file_io_flags & IO_OUT) {
					command->stdout_filename = init_word();
					add_letter_to_word(command->stdout_filename,ch);
				}
				return(command);
			}

		case STATE_REDIRECT_FILE :
			if(ch == '\n') {
				command->state = STATE_DONE;
				/* Turn off flags only used in parsing. */
				if(command->file_io_flags & IO_OUT);
					command->file_io_flags ^= IO_OUT;
				if(command->file_io_flags & IO_ERR);
					command->file_io_flags ^= IO_ERR;
				return(command);
			}
			if(ch == ' ' || ch == '\t') {
				command->state = STATE_SPACE;
				return(command);
			} else {
				if(command->file_io_flags & IO_OUT)
					add_letter_to_word(command->stdout_filename,ch);
				if(command->file_io_flags & IO_ERR) 
					add_letter_to_word(command->stderr_filename,ch);
				return(command);
			}

		case STATE_SEQ :
			if(ch == ' ' || ch == '\t' || ch == '\n') {
				next_command = init_command();
				next_command->text = command->text;
				command->pipeline = next_command;
				command->state = STATE_SPACE;
				return(next_command);
			}

			if(ch == '?') {
				command->flags |= FLAG_COND;
				return(command);
			}

			if(ch == '!') {
				command->flags |= FLAG_NEGCOND;
				return(command);
			}

			/* ERR */
			report_error("Unknown command sequence option",NULL,ch,0);
			return(NULL);

		case STATE_BACK :

			if(ch == '\n') {
				command->state = STATE_DONE;
				return(command);
			}

			if(ch == ' ' || ch == '\t') {
				command->state = STATE_BACK_POST;
				return(command);
			}

			if(ch == '-') {
				command->flags |= FLAG_NICE;
				command->nice += atoi(get_env("mpsh-nice"));
				return(command);
			}

			if(ch == '*') {
				command->flags |= FLAG_SMP;
				command->smp_num = 0;
				return(command);
			}
			if(ch == '!') {
				command->flags |= FLAG_SMP_SYM;
				return(command);
			}
			if(isdigit(ch)) {
				command->flags |= FLAG_SMP;
				command->smp_num *= 10;
				command->smp_num += ch - '0';
				return(command);
			}
			if(isalpha(ch)) {
				char tmp[32];
				command->job_handler = ch;
				command->flags |= FLAG_BATCH;
				sprintf(tmp,"handler-%c",command->job_handler);
				if(!get_env(tmp)) {
					report_error("Unknown job handler",
						NULL,command->job_handler,0);
					return(NULL);
				}
				return(command);
			}

			/* ERR */
			report_error("Unknown background option",NULL,ch,0);
			return(NULL);

		case STATE_BACK_POST : /* After "& ", looking for handler args */
			if(ch == '\n') {
				command->state = STATE_DONE;
				return(command);
			}
			if(ch == '|') {
				report_error("Cannot pipe from a background job. Use command grouping.",command->text,0,0);
				return(NULL);
			}
			if(ch == ';') {
				command->state = STATE_SEQ;
				return(command);
			}
			if(ch == ' ' || ch == '\t') {
				return(command);
			}
			command->state = STATE_BACK_ARGS;
			handler_args = new_word(&(command->handler_args));
			add_letter_to_word(handler_args,ch);
			return(command);

		case STATE_BACK_ARGS :
			if(ch == '\n') {
				command->state = STATE_DONE;
				return(command);
			}
			if(ch == ' ' || ch == '\t') {
				command->state = STATE_BACK_POST;
				return(command);
			}
			handler_args = find_last_word(&(command->handler_args));
			add_letter_to_word(handler_args,ch);
			return(command);


	}
}

expand_mp(command)
struct command *command;
{
	struct command *previous;

	previous = NULL;

	while(command) {
		if(command->flags & FLAG_SMP) { 
			/* Break this command into multiple */
			if(command->smp_num < 0 ||
					command->smp_num == 1) { /* Nope. */
				command->flags ^= FLAG_SMP;
				command->smp_num = 0;
				report_error("Multiprocess value must be 2 or higher",
					command->text,0,0);
				return(0);
				continue;
			}
			if(previous && (previous->flags & FLAG_PIPE)) {
				report_error("Use command grouping",command->text,0,0);
				return(0);
			}
			if(command->flags & FLAG_SMP_SYM)
				expand_mp_symmetric(command);
			else
				expand_mp_arguments(command);
		}
		previous = command;
		command = command->pipeline;
	}
	return(1);
}

expand_mp_symmetric(command)
struct command *command;
{
	int i, jobs;
	int cnt;
	int flags;
	struct command *next_command, *start_command, *cmd;
	struct command *last_command;
	struct word_list *w_start, *w;
	struct word_list *first_args, *original_args;
	int handler;
	int smp_id;
	int nice_val;

	command->smp_id = smp_id = unique_smp_id++;
	first_args = command->words;

	jobs = command->smp_num;

	if(jobs == 0) {
		for(w = command->words; w->next; w=w->next) jobs++;
		command->smp_num = jobs;
	}

	/* Save stuff from original command struct */
	flags = command->flags ^ FLAG_SMP ^ FLAG_SMP_SYM;
	command->flags = flags;
	handler = command->job_handler;
	original_args = command->words;
	next_command = command->pipeline;
	nice_val = command->nice;

	/* Start point of repeated loop. */
	start_command = command;

	/* Create jobs-1 new command structs */
	/* we're reusing the old struct, so -1 */
	for(i=1; i<jobs; i++) {
		command->pipeline = init_command();
		command->pipeline->text = command->text;
		command = command->pipeline;
		command->flags = flags;
		command->job_handler = handler;
		command->smp_id = smp_id;
		command->nice = nice_val;

		/* Copy arguments */
		copy_word_list(first_args,&command->words);
		copy_word_list(start_command->handler_args,&command->handler_args);

		/* copy redirect filenames */
		copy_word_list(start_command->stdout_filename,
			&command->stdout_filename);
		copy_word_list(start_command->stderr_filename,
			&command->stderr_filename);
	}

	last_command = command;

	last_command->pipeline = next_command;
}

expand_mp_arguments(command)
struct command *command;
{
	int i, jobs;
	int cnt;
	int flags;
	struct command *next_command, *start_command, *cmd;
	struct command *last_command;
	struct word_list *w_start, *w;
	struct word_list *first_args, *original_args;
	int handler;
	int smp_id;
	int nice_val;

	command->smp_id = smp_id = unique_smp_id++;
	first_args = w_start = command->words;
	command->words = NULL;
	if(w_start->next->word[0] == '-') { /* Two args */
		w_start = w_start->next->next;
		first_args->next->next = NULL;
	} else { /* One arg */
		w_start = w_start->next;
		first_args->next = NULL;
	}
	cnt = 0;
	w = w_start;
	while(w) {
		w = w->next;
		cnt++;
	}
	jobs = command->smp_num;
	if(jobs == 0) {
		jobs = cnt;
		command->smp_num = cnt;
	}
	if(cnt < jobs) jobs = cnt;

	/* Save stuff from original command struct */
	flags = command->flags ^ FLAG_SMP;
	handler = command->job_handler;
	nice_val = command->nice;
	original_args = command->words;
	next_command = command->pipeline;

	w = w_start;

	command->flags = flags;
	add_word(command,first_args);
	if(first_args->next) add_word(command,first_args->next);
	add_word(command,w);
	w = w->next;

	/* Start point of repeated loop. */
	start_command = command;

	/* Create jobs-1 new command structs */
	/* we're reusing the old struct, so -1 */
	for(i=1; i<jobs; i++) {
		command->pipeline = init_command();
		command->pipeline->text = command->text;
		command = command->pipeline;
		command->flags = flags;
		command->job_handler = handler;
		command->smp_id = smp_id;
		command->nice = nice_val;
		add_word(command,first_args);
		if(first_args->next) add_word(command,first_args->next);
		add_word(command,w);
		copy_word_list(start_command->handler_args,
			&command->handler_args);
		w = w->next;

		/* copy redirect filenames */
		copy_word_list(start_command->stdout_filename,
			&command->stdout_filename);
		copy_word_list(start_command->stderr_filename,
			&command->stderr_filename);
	}

	last_command = command;

	cmd = start_command;
	while(i < cnt) {
		add_word(cmd,w);
		w = w->next;
		cmd = cmd->pipeline;
		if(cmd == NULL) cmd = start_command;
		i++;
	}
	command = last_command;
	last_command->pipeline = next_command;
}

reset_command_text(command,pt)
struct command *command;
char *pt;
{
	strcpy(command->text,pt);
	strcat(command->text," ");
}

add_command_text_letter(command,ch)
struct command *command;
int ch;
{
	int len;

	len = strlen(command->text);
	command->text[len] = ch;
	command->text[len+1] = 0x00;
}

parse_word_list(orig_command,words)
struct command *orig_command;
struct word_list *words;
{
	struct word_list *w;
	char *pt;
	struct command *command;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(0);
	}

	command = orig_command;

	for(w=words; w; w=w->next) {
		for(pt=w->word; *pt; pt++) {
			command = parse_char(command,*pt);
			if(!command) return(0);
		}
		if(w->next)
			command = parse_char(command,' ');
		if(!command) return(0);
	}

	return(1);
}

struct command *parse_string(command,text)
struct command *command;
char *text;
{
	int state;
	char *str;
	struct command *ret;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(NULL);
	}

	command->words = NULL;
	command->text[0] = '\0';
	command->state = STATE_SPACE;

	for(str=text; *str; str++) {
		add_command_text_letter(command,*str);
		ret = parse_char(command,*str);
		if(!ret) return(0);
		command = ret;
	}

	add_command_text_letter(command,' ');
	command = parse_char(command,' ');

	return(command);
}

struct command *insert_parse_string(command,text)
struct command *command;
char *text;
{
	int state;
	char *str;
	struct command *ret;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(NULL);
	}

	command->state = STATE_WORD;

	for(str=text; *str; str++) {
		if(*str == '\n') 
			ret = parse_char_limited(command,' ');
		else 
			ret = parse_char_limited(command,*str);
		if(!ret) return(NULL);
		command = ret;
	}

	return(command);
}



struct command *parse_char_limited(command,ch)
struct command *command;
int ch;
{
	struct word_list *w, *ret_words;
	struct command *next_command;
	struct word_list *handler_args;
	char *alias_text;

	command->ch = ch;

	switch(command->state) {

		case STATE_WORD: /* Currently parsing a word */

			if(command->backslash) {
				command->backslash = 0;
				w = find_last_word(&command->words);
				add_letter_to_word(w,ch);
				return(command);
			}
			if(ch == '\\') {
				command->backslash = 1;
				return(command);
			}

			if(ch == '\"') {
				if(command->quote_stack[command->quote_depth] == QUOTE_DOUBLE) {
					command->quote_depth--;
					if(command->quote_depth < 1)
						return(command);
				} else {
					command->quote_stack[++command->quote_depth] =
						QUOTE_DOUBLE;
					if(command->quote_depth < 2)
						return(command);
				}
			}

			if(ch == '\'') {
				if(command->quote_stack[command->quote_depth] == QUOTE_SINGLE) {
					command->quote_depth--;
					if(command->quote_depth < 1)
						return(command);
				} else {
					command->quote_stack[++command->quote_depth] =
						QUOTE_SINGLE;
					if(command->quote_depth < 2)
						return(command);
				}
			}

			if(ch == '(') {
				command->quote_stack[++command->quote_depth] = QUOTE_PAREN;
				/*
				if(command->quote_depth == 1) {
					w = find_last_word(&command->words);
					command->start_of_sub = strlen(w->word);
				}
				*/
			}

			if(ch == ')') {
				if(command->quote_depth < 1 ||
					command->quote_stack[command->quote_depth] != QUOTE_PAREN) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->quote_depth--;
				/*
				if(command->quote_depth == 0) {
					char *str_to_exec;
					str_to_exec = find_last_word(&command->words)->word +
						command->start_of_sub;
					ret_words = words_from_command(str_to_exec+1);
					str_to_exec[0] = '\0';
					if(!parse_word_list(command,ret_words))
						return(NULL);
					return(command);
				}
				*/
			}

			if(ch == '[') {
				command->quote_stack[++command->quote_depth] = QUOTE_SQUARE;
			}

			if(ch == ']') {
				if(command->quote_depth < 1 ||
				command->quote_stack[command->quote_depth] != QUOTE_SQUARE) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->quote_depth--;
			}


			if(ch == '\n') {
				if(command->quote_depth > 0) {
					report_mismatched_something(command);
					return(NULL);
				}
				command->state = STATE_DONE;
				command->quote_depth = 0;
				if(command->words->next == NULL) {
					alias_text = find_alias(command->words->word);
					if(alias_text) {
						char *orig_str;
						orig_str = strdup(command->words->word);
						command = parse_string(command,alias_text);
						reset_command_text(command,orig_str);
						free(orig_str);
					}
				}
				return(command);
			}
			if((ch == ' ' || ch == '\t') && command->quote_depth == 0) {
				command->state = STATE_SPACE;
				if(command->words->next == NULL) {
					alias_text = find_alias(command->words->word);
					if(alias_text) {
						char *orig_str;
						orig_str = strdup(command->words->word);
						command = parse_string(command,alias_text);
						reset_command_text(command,orig_str);
						free(orig_str);
					}
				}
				return(command);
			}

			w = find_last_word(&command->words);
			add_letter_to_word(w,ch);
			return(command);

		case STATE_SPACE :

			if(ch == '\"') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_DOUBLE;
				w = new_arg_word(command);
				w->word[0] = 0;
				return(command);
			}

			if(ch == '\'') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_SINGLE;
				w = new_arg_word(command);
				w->word[0] = 0;
				return(command);
			}

			if(ch == '(') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_PAREN;
				/*
				command->start_of_sub = 0;
				*/
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

			if(ch == '[') {
				command->state = STATE_WORD;
				command->quote_stack[++command->quote_depth] = QUOTE_SQUARE;
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

			if(ch == ' ' || ch == '\t') {
				return(command);
			} else { 
				/* In a new word. */
				command->state = STATE_WORD;
				w = new_arg_word(command);
				w->word[0] = ch;
				w->word[1] = 0;
				return(command);
			}

	}
	perror("fell off end");
}

