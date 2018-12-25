# build testio executable when user executes "make"

appreg: node-manage.o aes.o cmac.o LoRaMacCrypto.o utilities.o tcp_client_fork.o tcp_server_fork.o LoRaDevOps.o GatewayPragma.o serverpkgformat.o mjmodbus.o lora_pkg.o mqtt.o
	$(CC) $(LDFLAGS) -lm -ljson-c -lpthread -lmosquitto -lmodbus node-manage.o aes.o cmac.o LoRaMacCrypto.o utilities.o tcp_client_fork.o tcp_server_fork.o LoRaDevOps.o GatewayPragma.o serverpkgformat.o mjmodbus.o lora_pkg.o mqtt.o -o node-manage

appreg.o: node-manage.c aes.c cmac.c LoRaMacCrypto.c utilities.c tcp_client_fork.c tcp_server_fork.c LoRaDevOps.c GatewayPragma.c serverpkgformat.c mjmodbus.c lora_pkg.c mqtt.c
	$(CC) $(CFLAGS) -c node-manage.c aes.c cmac.c LoRaMacCrypto.c utilities.c tcp_client_fork.c tcp_server_fork.c LoRaDevOps.c GatewayPragma.c serverpkgformat.c mjmodbus.c lora_pkg.c mqtt.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
