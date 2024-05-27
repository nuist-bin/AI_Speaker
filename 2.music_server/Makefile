Target=main
Object=main.o server.o database.o player.o

CXXFLAGS=-levent -lmysqlclient -ljsoncpp -std=c++11

$(Target):$(Object)
	g++ $(Object) -o $(Target) $(CXXFLAGS)

clean:
	rm $(Object) $(Target)
