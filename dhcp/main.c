#include <dhcpd.h>
int main(int argc, char const *argv[])
{
	/* code */
	struct dhcpd dd;
	init_dhcpd(dd);
	loop_dhcpd(dd);
	return 0;
}