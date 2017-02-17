
OBJS    = xmalloc.o initlib.o main.o  strlib.o ubus.o service.o

LIBS = -lubus -lubox 
BIN       =  systemd 




all: $(BIN)

clean:
	rm -f $(BIN)  *.o 

systemd: $(OBJS)  
	$(CC)  -o $(BIN) $(OBJS) $(LIBS) 

install: all
	cp -a $(BIN) $(DESTDIR)/usr/local/sbin 

unstall: all
	rm -f $(DESTDIR)/usr/local/sbin/$(BIN)
