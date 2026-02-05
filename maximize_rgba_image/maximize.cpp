#include <iostream>
#include <string>
#include <SFML/Graphics.hpp>

using std::string;

void usage(string programName) {
	std::cout << "Usage: " << programName << " [image file] [output image file]\n";
	std::cout << "Maximizes an 8-bit RGBA image file\n";
	std::cout << "Values below 127 become 0, otherwise 255,\n";
	std::cout << "irrespective of color channel.\n";
}

inline sf::Uint8 maximize_channel(sf::Uint8 number) {
	if (number < 127) {
		return 0;
	}

	return 255;
}

void maximize_image(sf::Image *img) {
	if (img == nullptr) {
		std::cerr << "maximize_image(): empty, nullptr image\n";
		return;
	}

	for (size_t y = 0; y < img->getSize().y; y++) {
		for (size_t x = 0; x < img->getSize().x; x++) {
			sf::Color color = img->getPixel(x, y);
			color.r = maximize_channel(color.r);
			color.g = maximize_channel(color.g);
			color.b = maximize_channel(color.b);
			color.a = maximize_channel(color.a);
			img->setPixel(x, y, color);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage(argv[0]);
		return 0;
	}

	sf::Image img;
	img.loadFromFile(argv[1]);

	maximize_image(&img);

	img.saveToFile(argv[2]);
}
