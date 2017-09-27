# build testio executable when user executes "make"

appreg: node-manage.o
	$(CC) $(LDFLAGS) node-manage.o -o node-manage

appreg.o: node-manage.c
	$(CC) $(CFLAGS) -c node-manage.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
