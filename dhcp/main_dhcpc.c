#include "dhcpc.h"
#include <stdio.h>
int main(int argc, char *argv[])
{
	struct dhcpc dhc;
	init_dhcpc(&dhc, argc, argv);
	return 0;
}