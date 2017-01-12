CC := gcc

ALL := SERVER CLIENT
SERVER := mydhcpd
CLIENT := mydhcpc

.PHONY: all clean
all: $(ALL)

SERVER: dhcp_packet.c dhcpd.c main_dhcpd.c ip_list.c centry.c
	$(CC)  dhcp_packet.c dhcpd.c main_dhcpd.c ip_list.c centry.c -o mydhcpd
CLIENT: dhcpc.c dhcp_packet.c main_dhcpc.c
	$(CC)  dhcpc.c dhcp_packet.c main_dhcpc.c -o mydhcpc
clean:
	rm $(OUTPUT)
