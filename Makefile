all:
	gcc s-talk.c list.c list.h -o s-talk -lpthread -lnsl 

clean:
	rm -f s-talk