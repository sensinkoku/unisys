CC := gcc

ALL := SERVER CLIENT
SERVER := myftpd.out
CLIENT := myftpc.out

.PHONY: all clean
all: $(ALL)

SERVER: myftpd.c
	$(CC)  -g myftpd.c -o myftpd.out
CLIENT: myftpc.c
	$(CC)  -g myftpc.c -o myftpc.out
clean:
	rm $(OUTPUT)
