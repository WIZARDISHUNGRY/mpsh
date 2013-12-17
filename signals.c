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

signals.c:



*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>


#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"

extern int current_job_pid;


static void sig_int(s)
int s;
{
	int p;
	signal(SIGINT,sig_int);
	p = current_job_pid;
	if(p != -1) kill(p,SIGINT);
}


static void sig_stop(s)
int s;
{
	int p;
	signal(SIGTSTP,sig_stop);
	p = current_job_pid;
	if(p != -1) kill(p,SIGTSTP);
}

static void sig_cont(s)
int s;
{
	int p;
	signal(SIGCONT,sig_cont);
	p = current_job_pid;
	if(p != -1) kill(p,SIGCONT);
}

static void sig_out(s)
int s;
{
	signal(SIGTTOU,sig_out);
}


init_signals() {
	signal(SIGINT,sig_int);
	signal(SIGTSTP,sig_stop);
	/*
	signal(SIGCONT,sig_cont);
	*/
	signal(SIGTTOU,sig_out);
}




