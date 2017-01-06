#include <input_line.h>
#include <stdio.h>
#include <string.h>
//Token status
#define ISPIPE 1 // |
#define ISREDIRECTRIGHT 2 // <
#define ISREDIRECTLEFT 3 // >
#define ISANPASAND 4 // &
#define ISNORMAL 0 // The others

#define PIPENUMERR
#define POSITIONERR

//Prottype declaeration
static void print_line_err (char * msg, int status);
static int check_token_status (struct input_line *);
static int count_pipenum(char *line);
static int parse_line_to_process(struct input_line *);
// Define extern function
int input_line_init (struct input_line * data, int argc, char **argv) {
	data->tokens = argv;
	data->tokennum = acrg;
	data->pipenum = count_pipenum(data->tokens, data->tokennum);
	check_token_status (data->tokendata);



}

//Define static functions
static int parse_line_to_process(struct input_line * line) {
	int startindex = 0;
	int endindex;
	int tmpindex = 0;
	int comlength;

	int psnum = 0;
	int input, output;

	int i, j;
	int start, end;
	for (psnum = 0; psnum < line->pipenum + 1; psnum++) {
		while (strcmp(line->tokens[tmpindex], "|") != 0) tmpindex++;
		endindex = tmpindex-1;
		comlength = endindex - startindex;
		//pipecheck
		if (psnum == 0) input = MYSTDIN;
		else input = STDINPIPE;
		if (psnum == line->pipenum) output = MYSTDOUT;
		else output = STDOUTPIPE;
		char * buf[comlength];
		int i;
		for (i = startindex; i <= endindex; i++) {
			strcpy(buf[i-startlength], line->tokens[i]);
		}
		input_token_init(line->ps[psnum], buf, comlength, input, output);
	}
}

static int check_token_status (struct input_ine * data) {
	int i;
	char * token;
	for (i = 0; i < data->tokennum; i++) {
		token = data->tokens[i];
		if (strcmp(token,"|") == 0) data->tokendata[i] = ISPIPE;
		else if (strcmp(token,">") == 0) data->tokendata[i] = ISREDIRECTLEFT;
		else if (strcmp(token,"<") == 0) data->tokendata[i] = ISREDIRECTRIGHT;
		else if (strcmp(token,"&") == 0) data->tokendata[i] = ISANPASAND;
		else data->tokendata[i] = ISNORMAL;
	}
	return 0;
}

static int count_pipenum (char ** tokens, int tokennum) {
	int pipenum == 0;
	int i;
	for (i = 0; i < tokennum; i++) {
		if (strcmp(tokens[i], "|") == 0) pipenum++;
	}
	return pipenum;
}



