bin/server: main.cpp
	- mkdir -p bin
	- g++ -std=c++14 main.cpp -o bin/server

clean:
	- rm -rf bin
