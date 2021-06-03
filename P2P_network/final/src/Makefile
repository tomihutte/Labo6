CC			= gcc
CFLAGS 		= -g -Wall -Werror 
LDFLAGS		= -lpthread -lssl -lcrypto
TARGETS 	= tracker_files client_seeder client_peer client_peer2

all:	$(TARGETS)

tracker_files: 
	gcc $(CFLAGS) tracker_files.c include/server.c include/proto.c include/meta_file.c -o tracker_files $(LDFLAGS)

client_seeder: 
	gcc  $(CFLAGS) client_seeder.c include/peer.c include/proto.c include/meta_file.c -o ../seeder/client_seeder $(LDFLAGS)

client_peer: 
	gcc $(CFLAGS) client_peer.c include/peer.c include/proto.c include/meta_file.c -o ../peer1/client_peer $(LDFLAGS)

client_peer2:
	gcc $(CFLAGS) client_peer.c include/peer.c include/proto.c include/meta_file.c -o ../peer2/client_peer $(LDFLAGS)

clean:
	rm -f $(TARGETS) core* *.o
	find ../ -name client_peer -delete
	find ../ -name client_seeder -delete
	find ../ -name tracker_files -delete
