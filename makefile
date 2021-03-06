all: peer tracker tgen filegen

peer_base.o: peer.h peer_base.c
	gcc peer_base.c -c -g -o peer_base.o

peer_active.o: peer.h peer_active.c
	gcc peer_active.c -c -g -o peer_active.o

peer_passive.o: peer.h peer_passive.c
	gcc peer_passive.c -c -g -o peer_passive.o

peer_tracker.o: peer_tracker.c peer.h
	gcc peer_tracker.c -c -g -o peer_tracker.o

peer_cmd.o: peer_cmd.c peer.h
	gcc peer_cmd.c -c -g -o peer_cmd.o

peer.o: peer.c peer.h
	gcc peer.c -c -g -lpthread -o peer.o

peer: peer_base.o peer_active.o peer_passive.o peer_tracker.o peer_cmd.o peer.o peer.h
	gcc peer_base.o peer_active.o peer_passive.o peer_tracker.o peer_cmd.o peer.o -g -o peer -lpthread

tracker: tracker.c
	gcc tracker.c -g -o tracker -lpthread -std=gnu99

tgen: tgen.c
	gcc tgen.c -g -o tgen

filegen: filegen.c
	gcc filegen.c -g -o filegen

clean:
	rm -f tracker tgen *.o peer filegen 

