CC=gcc
CC_OPTIONS=-Wall -O3 -lsqlite3 -ljson -lgps

nerdobd2 : core

# core
core : core.o kw1281.o sqlite.o httpd.o db2json.o json.o tcp.o gps.o
	$(CC) -o nerdobd2 core.o kw1281.o sqlite.o db2json.o httpd.o tcp.o json.o gps.o $(CC_OPTIONS)

core.o : core.c
	$(CC) $(CC_OPTIONS) -c core.c -o core.o
	
kw1281.o : kw1281.c
	$(CC) $(CC_OPTIONS) -c kw1281.c -o kw1281.o


# http server
httpd.o : httpd.c
	$(CC) $(CC_OPTIONS) -c httpd.c -o httpd.o
	
db2json.o : db2json.c
	$(CC) $(CC_OPTIONS) -c db2json.c -o db2json.o


# sqlite access
sqlite.o : sqlite.c
	$(CC) $(CC_OPTIONS) -c sqlite.c -o sqlite.o

# tcp helpers
tcp.o : tcp.c
	$(CC) $(CC_OPTIONS) -c tcp.c -o tcp.o

# json helpers
json.o : json.c
	$(CC) $(CC_OPTIONS) -c json.c -o json.o

# gps support
gps.o : gps.c
	$(CC) $(CC_OPTIONS) -c gps.c -o gps.o


# cleaning	
clean :
	rm -f *.o nerdobd2

