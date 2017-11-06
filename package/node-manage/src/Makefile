# build testio executable when user executes "make"

appreg: node-manage.o LoRaMac.o aes.o cmac.o LoRaMacCrypto.o Region.o RegionCN470.o RegionCommon.o routin.o utilities.o tcp_client_fork.o tcp_server_fork.o sx1276.o
	$(CC) $(LDFLAGS) -lm -lpthread node-manage.o LoRaMac.o aes.o cmac.o LoRaMacCrypto.o Region.o RegionCN470.o RegionCommon.o routin.o utilities.o tcp_client_fork.o tcp_server_fork.o sx1276.o -o node-manage

appreg.o: node-manage.c LoRaMac.c aes.c cmac.c LoRaMacCrypto.c Region.c RegionCN470.c RegionCommon.c routin.c utilities.c tcp_client_fork.c tcp_server_fork.c sx1276.c
	$(CC) $(CFLAGS) -c node-manage.c LoRaMac.c aes.c cmac.c LoRaMacCrypto.c Region.c RegionCN470.c RegionCommon.c routin.c utilities.c tcp_client_fork.c tcp_server_fork.c sx1276.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
