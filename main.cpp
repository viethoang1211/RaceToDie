#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include"bits/stdc++.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>

#ifdef WIN32
    #include <winsock.h>
	#include <winsock2.h>
    typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <sys/un.h>
#endif
#include <unistd.h>
#include "player.cpp"
#include "message.cpp"
using namespace std;



class Textbox : public sf::Drawable, public sf::Transformable {
public:
	Textbox(unsigned int len) :
			max_len(len),
            my_rect(sf::Vector2f(30 * max_len, 40)), 
            my_focus(true)
        {
            my_font.loadFromFile("C:/Windows/Fonts/Arial.ttf"); // change later
			my_text.setFont(my_font);
			my_text.setColor(sf::Color::Black);
			my_text.setString("_");

            my_rect.setOutlineThickness(2);
            my_rect.setFillColor(sf::Color::White);
            my_rect.setOutlineColor(sf::Color(127,127,127)); //gray
            my_rect.setPosition(0,0);

        }

	void setPosition(float x, float y){
		sf::Transformable::setPosition(x, y);
		my_text.setPosition(x, y);
		my_rect.setPosition(x, y);
	}

	void input(sf::Event e){
		if (!my_focus || e.type != sf::Event::TextEntered)
			return;

		if (e.text.unicode == 8){   // Delete key
			text = text.substr(0, text.size() - 1);
			my_text.setString(text+ "_");
		}
		else if (text.size() < max_len && e.text.unicode != 13){
			text+= e.text.unicode;
			my_text.setString(text+ "_");
		}
	}

	void setFocus(bool focus){
		my_focus = focus;
		if (focus){
			my_rect.setOutlineColor(sf::Color::Blue);
		}
		else{
			my_rect.setOutlineColor(sf::Color(127, 127, 127)); // Gray color
		}
	}

	string getString(){
		return text;
	}

	void setString(string s){
		my_text.setString(s);
	}

	void setFont(sf::Font &fonts) {
		my_text.setFont(fonts);
	}

	bool contains(sf::Vector2f point) {
		return my_rect.getGlobalBounds().contains(point);
	}
private:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= this->getTransform();
        target.draw(my_rect);
        target.draw(my_text);
    }
	unsigned int max_len;
	string text;
	sf::Text my_text;
	sf::Font my_font;
	sf::RectangleShape my_rect;
	bool my_focus;
};
string game_input_name(sf::RenderWindow& window){
	Textbox tb1(20);
	tb1.setPosition(100,100);
	sf::Event event;
	while (window.isOpen()) {
		while (window.pollEvent(event)){
			if (event.type == sf::Event::Closed)
				window.close();
			else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)){
				return tb1.getString();
			}	
			// else if (event.type == sf::Event::MouseButtonReleased){
			// 	auto pos = sf::Mouse::getPosition(window);
			// 	tb1.setFocus(false);
			// 	if (tb1.contains(sf::Vector2f(pos))){
			// 		tb1.setFocus(true);
			// 	}
			// }
			else{
				tb1.input(event);
			}
		}
		window.clear();
		window.draw(tb1);
		window.display();
	}
	return "";
}
int game(sf::RenderWindow& window){
	sf::Image image;
	image.loadFromFile("obs3.png");
	image.createMaskFromColor(sf::Color::White);
	sf::Texture car;
	car.loadFromImage(image);
	sf::Sprite cars(car);
	
	vector<sf::Sprite> carss(10,cars);
	for(int i=0;i<10;i++){
		carss[i].setPosition(15+105*i,825-120);
	}

	sf::Texture race;
	if(!race.loadFromFile("a.png"))
		return EXIT_FAILURE;
	sf::Sprite	racing;
	racing.setTexture(race);

	sf::Event event;
	while (window.isOpen()) {
		while (window.pollEvent(event)){
			if (event.type == sf::Event::Closed)
				window.close();
			else if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
				window.close();
			}	
		}
		window.clear();
		for(int i=0;i<10;i++){
			racing.setPosition(105*i,0);
			window.draw(racing);
			window.draw(carss[i]);
		}
		window.display();
	}
	return 0;
}
int main() {
	sf::RenderWindow window;
	window.create(sf::VideoMode(1200, 900), "SFML Textbox");

	if(game_input_name(window)=="testt"){
		return game(window);
	};    

	return 0;
}