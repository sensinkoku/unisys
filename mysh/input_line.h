#ifndef _INPUT_LINE_H_
#define _INPUT_LINE_H_
struct input_token;
struct input_line {
	int tokennum;
	char ** tokens;
	int pipenum;
	int tokendata[tokennum];
	struct input_process *ps[pipenum+1];
};

extern int input_line_init (struct input_line * data, int argc, char ** argv);

#endif // _INPUT_LINE_H_