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

init.c:

	initialization functions

	(Some initializations functions are in their relevant
	files instead.)

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifdef LINUX
#endif

#ifdef BSD
#endif

#include "mpsh.h"



init_all(argc,argv,env)
int argc;
char *argv[];
char *env[];
{
	char buff[256];
	char *home;
	int ret;


	init_terminal();

	parse_depth = 0;

	/* Init environment variables & search path */
	init_global_env(env);
	init_command_path_list();
	init_search_path();
	update_search_path();
	init_internal_env();

	/* Init History */
	init_history();
	init_cdhistory();

	init_jobs();
	init_signals();
	init_prompt();

	init_smp_id();
	init_error_level();

	if(argc == 1 || strcmp(argv[1],"-n") != 0) {
		home = get_env("HOME");
		if(home && home != (char *) -1) {
			sprintf(buff,"%s/.mpshrc_all",home);
			run_script(buff,1);
			if(argv[0][0] == '-') { /* Login shell */
				sprintf(buff,"%s/.mpshrc_login",home);
				run_script(buff,1);
			}
		}
	}

	if(argc == 1) {
		return;
	}

	if(strcmp(argv[1],"-c") == 0) {
		parse_and_run(argv[2],INTERACTIVE);
		exit(0);
	}

	ret = run_script_with_args(argc,argv,0);
	exit(ret);
}

