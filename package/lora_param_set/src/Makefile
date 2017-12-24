# build testio executable when user executes "make"
JSONC_LDFLAGS = -lm -ljson-c

appreg: lora_param_set.o
	$(CC) $(JSONC_CFLAGS) $(LDFLAGS) $(JSONC_LDFLAGS) lora_param_set.o -o lora_param_set

appreg.o: lora_param_set.c
	$(CC) $(CFLAGS) -c lora_param_set.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
