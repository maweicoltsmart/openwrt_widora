# build testio executable when user executes "make"

appreg: lora_factory_reset.o
	$(CC) $(JSONC_CFLAGS) $(LDFLAGS) $(JSONC_LDFLAGS) lora_factory_reset.o -o lora_factory_reset

appreg.o: lora_factory_reset.c
	$(CC) $(CFLAGS) -c lora_factory_reset.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
