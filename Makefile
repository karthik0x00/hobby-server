bin/server: main.cpp
	- mkdir -p bin
	- g++ -std=c++11 main.cpp -o bin/server

clean:
	- rm -rf bin
