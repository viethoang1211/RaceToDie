all: server2compile server2link client2compile client2link
server2compile:
	g++ -c server2.cpp -IC:\Users\ADMIN\Documents\SFML-2.5.1\include

server2link:
	g++ server2.o -o server2 -lws2_32 -LC:\Users\ADMIN\Documents\SFML-2.5.1\lib -lsfml-graphics -lsfml-window -lsfml-system

client2compile :
	g++ -c client.cpp -IC:\Users\ADMIN\Documents\SFML-2.5.1\include

client2link:
	g++ client.o -o client -lws2_32 -LC:\Users\ADMIN\Documents\SFML-2.5.1\lib -lsfml-graphics -lsfml-window -lsfml-system