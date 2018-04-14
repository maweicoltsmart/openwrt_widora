# build testio executable when user executes "make"

appreg: node-manage.o aes.o cmac.o LoRaMacCrypto.o utilities.o mqtt.o tcp_client_fork.o tcp_server_fork.o LoRaDevOps.o GatewayPragma.o serverpkgformat.o
	$(CC) $(LDFLAGS) -lm -ljson-c -lpthread -lmosquitto node-manage.o aes.o cmac.o LoRaMacCrypto.o utilities.o mqtt.o tcp_client_fork.o tcp_server_fork.o LoRaDevOps.o GatewayPragma.o serverpkgformat.o -o node-manage

appreg.o: node-manage.c aes.c cmac.c LoRaMacCrypto.c utilities.c mqtt.c tcp_client_fork.c tcp_server_fork.c LoRaDevOps.c GatewayPragma.c serverpkgformat.c
	$(CC) $(CFLAGS) -c node-manage.c aes.c cmac.c LoRaMacCrypto.c utilities.c mqtt.c tcp_client_fork.c tcp_server_fork.c LoRaDevOps.c GatewayPragma.c serverpkgformat.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
