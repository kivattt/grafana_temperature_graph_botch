#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>

#include "webcam-v4l2/webcam.h"

#define WIDTH 640
#define HEIGHT 480

sf::Image texture_transformed_as_image(sf::Texture &txt, float x, float y, float angle, float scaleX, float scaleY) {
	sf::RenderTexture renderTxt;
	unsigned width = txt.getSize().x;
	unsigned height = txt.getSize().y;

	renderTxt.create(width, height);
	renderTxt.clear(sf::Color::Transparent);

	sf::Sprite spr;
	spr.setTexture(txt);

	// Transform
	spr.setRotation(angle);
	spr.setScale(scaleX, scaleY);
	spr.setPosition(x, y);

	renderTxt.draw(spr);
	renderTxt.display();
	return renderTxt.getTexture().copyToImage();
}

int main() {
	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "camera", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	Webcam webcam("/dev/video2", WIDTH, HEIGHT);

	sf::Texture maskTxt; // TODO load img
	maskTxt.loadFromFile("mask/mask_maximized.png");

	bool isMaskVisible = true;
	float maskPosX = 0.0;
	float maskPosY = 0.0;
	float maskRotation = 0.0;
	float maskScaleX = 0.2;
	float maskScaleY = 0.2;

	while (window.isOpen()) {
		sf::Event e;
		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed) {
				window.close();
			} else if (e.type == sf::Event::KeyPressed) {
				if (e.key.code == sf::Keyboard::Key::Space) {
					// Toggle visibility of the mask
					isMaskVisible = !isMaskVisible;
				}
			}
		}

		float speed = 3.0;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) speed = 0.75;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) maskPosY -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) maskPosX -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) maskPosY += speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) maskPosX += speed;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) maskRotation -= speed * 0.25;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) maskRotation += speed * 0.25;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
			maskScaleX += speed * 0.001;
			maskScaleY += speed * 0.001;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
			maskScaleX -= speed * 0.001;
			maskScaleY -= speed * 0.001;
		}

		// Clamp the scale
		maskScaleX = std::max(0.01f, maskScaleX);
		maskScaleY = std::max(0.01f, maskScaleY);

		window.clear();

		auto frame = webcam.frame();
		sf::Image frameImg;
		frameImg.create(WIDTH, HEIGHT, (sf::Uint8*)frame.data);
		sf::Texture txt;
		txt.loadFromImage(frameImg);
		sf::Sprite spr;
		spr.setTexture(txt);

		sf::Image maskImg = texture_transformed_as_image(maskTxt, maskPosX, maskPosY, maskRotation, maskScaleX, maskScaleY);
		sf::Texture maskImgTxt;
		maskImgTxt.loadFromImage(maskImg);

		sf::Sprite maskSpr;
		maskSpr.setTexture(maskImgTxt);
		maskSpr.setColor({255, 255, 255, 100});

		window.draw(spr);
		if (isMaskVisible) window.draw(maskSpr);
		window.display();
	}
}
