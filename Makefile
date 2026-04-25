CC=gcc
CFLAGS=-O2 -Wall

TOOLS=delegation_scan delegation_scan2 shared_inode_scan lifecycle_test persistence_test process_matrix_scan

all: $(TOOLS)

%: tools/%.c
	$(CC) $(CFLAGS) -o tools/$@ tools/$<

clean:
	rm -f tools/delegation_scan tools/delegation_scan2 tools/shared_inode_scan tools/lifecycle_test tools/persistence_test tools/process_matrix_scan

.PHONY: all clean
