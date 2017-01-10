#include "dhcpd.h"
int main(int argc, char const *argv[])
{
	struct dhcpd dd;
	init_dhcpd(&dd, argc, argv);
	loop_dhcpd(&dd);
	return 0;
}