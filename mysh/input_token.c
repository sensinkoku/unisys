#include <input_token.h>

//Prototype declaeration
static int check_redirect(struct input_process *);
static int is_background();

//Define extern functions
int input_token_init(struct input_process * ps, char ** input, int num, int ispipein, int ispipeout) {
	ps->ac = num;
	ps->stdin = ispipein;
	ps->stdout = ispipeout;
	int i;
	for (i = 0; i < ps->ac; i++) {
		ps->argv[i] = input[i];
	}

}
// Define static functions
static int check_redirect(struct input_process * ps) {

}
