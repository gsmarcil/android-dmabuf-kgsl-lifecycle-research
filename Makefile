CC=clang
CFLAGS=-O2 -Wall -I.
TOOLS=delegation_scan delegation_scan2 lifecycle_test \
      persistence_test shared_inode_scan process_matrix_scan

all: $(TOOLS)

%: tools/%.c
	$(CC) $(CFLAGS) -o tools/$@ $<

clean:
	rm -f $(addprefix tools/,$(TOOLS))

.PHONY: all clean
