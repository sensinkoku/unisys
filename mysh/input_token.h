#ifndef _INPUT_TOKEN_H_
#define _INPUT_TOKEN_H_


#define STDOUTPIPE
#define STDINPIPE
#define STDOUTREDIRECT
#define MYSTDIN
#define MYSTDOUT


struct input_line;
struct input_process {
	//struct input_line * line;
	int ac;
	char *argv[ac];
	int stdin;
	int stdout;
	int inputfileindex;
	int outputfileindex;
	//int isbackground;
	//int pid;
	//int ppid;
};
extern int input_token_init(struct input_process *, char ** input, int num, int ispipein, int ispipeout);


#endif // _INPUT_TOKEN_H_
