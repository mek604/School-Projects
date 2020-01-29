.phony all:
all: program

program: program.c
	gcc program.c -lreadline -lhistory -ltermcap -o program

.PHONY clean:
clean:
	-rm -rf *.o *.exe

