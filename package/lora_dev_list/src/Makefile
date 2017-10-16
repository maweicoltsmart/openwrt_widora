# build testio executable when user executes "make"
JSONC_LDFLAGS = -lm -ljson-c

appreg: lora_dev_list.o
	$(CC) $(JSONC_CFLAGS) $(LDFLAGS) $(JSONC_LDFLAGS) lora_dev_list.o -o lora_dev_list

appreg.o: lora_dev_list.c
	$(CC) $(CFLAGS) -c lora_dev_list.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
