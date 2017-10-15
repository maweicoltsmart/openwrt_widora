# build testio executable when user executes "make"

appreg: node-manage.o LoRaMac.o aes.o cmac.o LoRaMacCrypto.o Region.o RegionCN470.o RegionCommon.o routin.o utilities.o
	$(CC) $(LDFLAGS) -lm -lpthread node-manage.o LoRaMac.o aes.o cmac.o LoRaMacCrypto.o Region.o RegionCN470.o RegionCommon.o routin.o utilities.o -o node-manage

appreg.o: node-manage.c LoRaMac.c aes.c cmac.c LoRaMacCrypto.c Region.c RegionCN470.c RegionCommon.c routin.c utilities.c
	$(CC) $(CFLAGS) -c node-manage.c LoRaMac.c aes.c cmac.c LoRaMacCrypto.c Region.c RegionCN470.c RegionCommon.c routin.c utilities.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o appreg
