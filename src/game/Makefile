
JUNIXSOCKET=./junixsocket-1.3/
LIB=$(JUNIXSOCKET)/lib-native/
JUNIXSOCKET_JAR=$(JUNIXSOCKET)/dist/junixsocket-1.3.jar
SOCK?=/tmp/test.sock

all: TestGame

TestGame: TestGame.java
	javac TestGame.java -d . -classpath $(JUNIXSOCKET_JAR)

run:
	java -D  -Dorg.newsclub.net.unix.library.path=$(LIB) -cp .:$(JUNIXSOCKET_JAR)  wg.games.TestGame $(SOCK)
	
clean:
	-rm TestGame.class

