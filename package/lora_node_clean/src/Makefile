# build testio executable when user executes "make"

appreg: lora_node_clean.o
	$(CC) $(JSONC_CFLAGS) $(LDFLAGS) $(JSONC_LDFLAGS) lora_node_clean.o -o lora_node_clean

appreg.o: lora_node_clean.c
	$(CC) $(CFLAGS) -c lora_node_clean.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
