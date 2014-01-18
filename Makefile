# Specify your operating system:
# (Use BSD for OSX.)
OS=Solaris
#OS=LINUX
#OS=IRIX
#OS=BSD

# Comment these if you don't want readline.
READLINE=-DREADLINE
LIBS=-lreadline -lcurses

# Specify your compiler:
# (OSX must use gcc, not clang! clang masquerading as gcc will not work!)
CC=gcc
#CC=cc


OBJECTS=main.o parse.o fns.o glob.o prompt.o debug.o exec.o env.o jobs.o \
		builtin.o history.o signals.o chdir.o init.o set-theory.o \
		error.o alias.o scripts.o


mpsh: $(OBJECTS)
	$(CC) $(OBJECTS) -o mpsh $(LIBS)

main.o: main.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c main.c

parse.o: parse.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c parse.c

fns.o: fns.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c fns.c

glob.o: glob.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c glob.c

prompt.o: prompt.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c prompt.c

debug.o: debug.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c debug.c

exec.o: exec.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c exec.c

env.o: env.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c env.c

jobs.o: jobs.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c jobs.c

builtin.o: builtin.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c builtin.c

history.o: history.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c history.c

signals.o: signals.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c signals.c

chdir.o: chdir.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c chdir.c

init.o: init.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c init.c

set-theory.o: set-theory.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c set-theory.c

error.o: error.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c error.c

alias.o: alias.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c alias.c

scripts.o: scripts.c mpsh.h
	$(CC) -D$(OS) $(READLINE) -c scripts.c

