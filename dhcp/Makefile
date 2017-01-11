CC := gcc

ALL := SERVER CLIENT
SERVER := server.out
CLIENT := client.out

.PHONY: all clean
all: $(ALL)

SERVER: dhcp_packet.c dhcpd.c main_dhcpd.c ip_list.c centry.c
	$(CC) -g dhcp_packet.c dhcpd.c main_dhcpd.c ip_list.c centry.c -o server.out
CLIENT: dhcpc.c dhcp_packet.c main_dhcpc.c
	$(CC) -g dhcpc.c dhcp_packet.c main_dhcpc.c -o client.out
clean:
	rm $(OUTPUT)