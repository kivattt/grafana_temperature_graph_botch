#include <iostream>
#include <fcntl.h>
#include <string>
#include <algorithm>
#include <math.h>
#include <SFML/Graphics.hpp>

#include "util.hpp"
#include "webcam-v4l2/webcam.h"

#define WIDTH 640
#define HEIGHT 480

using std::string;

struct MaskPos {
	float x = 0.0;
	float y = 0.0;
	float scaleX, scaleY;
	float width, height;
	float angle = 0.0;
};

void read_maskpos_from_file(MaskPos &m, string filename) {
	int fd = open(filename.c_str(), O_RDONLY, 0664);
	if (fd < 0) {
		// Default values
		return;
	}

	read(fd, &m, sizeof(MaskPos));
	close(fd);
}

void write_maskpos_to_file(MaskPos &m, string filename) {
	int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0664);
	write(fd, &m, sizeof(MaskPos));
	close(fd);
}

void set_mask_array_stuff(sf::VertexArray &arr,
                          sf::Texture texture,
                          sf::Color color,
                          float x,
                          float y,
                          float width,
                          float height,
                          float rotation
) {
	arr[0].position = sf::Vector2f(x, y);
	arr[1].position = sf::Vector2f(x+width, y);
	arr[2].position = sf::Vector2f(x, y+height);
	arr[3].position = sf::Vector2f(x+width, y+height);

	arr[0].color = color;
	arr[1].color = color;
	arr[2].color = color;
	arr[3].color = color;

	arr[0].texCoords = sf::Vector2f(0, 0);
	arr[1].texCoords = sf::Vector2f(texture.getSize().x, 0);
	arr[2].texCoords = sf::Vector2f(0, texture.getSize().y);
	arr[3].texCoords = sf::Vector2f(texture.getSize().x, texture.getSize().y);
}

sf::Vector2f middle_of_four_points(sf::VertexArray &arr) {
	float maxX = std::max({arr[0].position.x, arr[1].position.x, arr[2].position.x, arr[3].position.x});
	float minX = std::min({arr[0].position.x, arr[1].position.x, arr[2].position.x, arr[3].position.x});
	float x = 0.5 * (maxX + minX);

	float maxY = std::max({arr[0].position.y, arr[1].position.y, arr[2].position.y, arr[3].position.y});
	float minY = std::min({arr[0].position.y, arr[1].position.y, arr[2].position.y, arr[3].position.y});
	float y = 0.5 * (maxY + minY);

	return sf::Vector2f(x, y);
}

float angle_between_two_points(sf::Vector2f base, sf::Vector2f point) {
	float dx = point.x - base.x;
	float dy = point.y - base.y;

	if (point.x > base.x) {
		return atan(dy / dx);
	} else {
		return M_PI + atan(dy / dx);
	}
}

void compute_angles_to_middle(sf::VertexArray &arr, sf::Vector2f middle, float *pt0Angle, float *pt1Angle, float *pt2Angle, float *pt3Angle) {
	*pt0Angle = angle_between_two_points(middle, arr[0].position);
	*pt1Angle = angle_between_two_points(middle, arr[1].position);
	*pt2Angle = angle_between_two_points(middle, arr[2].position);
	*pt3Angle = angle_between_two_points(middle, arr[3].position);
}

void compute_radius_to_middle(sf::VertexArray &arr, sf::Vector2f middle, float *pt0Radius, float *pt1Radius, float *pt2Radius, float *pt3Radius) {
	*pt0Radius = hypot(arr[0].position, middle);
	*pt1Radius = hypot(arr[0].position, middle);
	*pt2Radius = hypot(arr[0].position, middle);
	*pt3Radius = hypot(arr[0].position, middle);
}

//sf::Image texture_transformed_as_image(sf::Texture &txt, float maskWidth, float maskHeight, float x, float y, float scaleX, float scaleY, float angle) {
sf::Image texture_transformed_as_image(sf::Texture &txt, MaskPos mask) {
	sf::RenderTexture renderTxt;
	renderTxt.create(WIDTH, HEIGHT);
	renderTxt.clear(sf::Color::Transparent);

	sf::VertexArray maskArray(sf::PrimitiveType::TriangleStrip, 4);
	sf::Color maskColor(255, 255, 255, 255);
	set_mask_array_stuff(maskArray, txt, maskColor, mask.x, mask.y, mask.scaleX*mask.width, mask.scaleY*mask.height, mask.angle);

	/* TODO: Shouldn't all this rotation stuff be inside set_mask_array_stuff() ? */
	sf::Vector2f midPoint = middle_of_four_points(maskArray);
	float radius = 2.0;
	sf::CircleShape circle(radius);
	circle.setFillColor(sf::Color(255, 0, 0, 255));
	circle.setPosition(midPoint - sf::Vector2f(radius, radius));

	// Recompute angle every time.
	// This way, I think mask.scaleX and mask.scaleY differing would still keep the angle constant.

	/*
	 * This illustration shows the order of the points in a 0-degree rectangle.
	 *
	 *   pt.0          pt.1
	 *    +-------------+
	 *    |             |
	 *    |     mid     |
	 *    |             |
	 *    +-------------+
	 *   pt.2          pt.3
	 *
	 *
	 *
	 *   "pt." is shorthand for point
	*/

	float pt0Angle, pt1Angle, pt2Angle, pt3Angle;
	compute_angles_to_middle(maskArray, midPoint, &pt0Angle, &pt1Angle, &pt2Angle, &pt3Angle);
	float pt0Radius, pt1Radius, pt2Radius, pt3Radius;
	compute_radius_to_middle(maskArray, midPoint, &pt0Radius, &pt1Radius, &pt2Radius, &pt3Radius);

	// We need the expected angles for a 0-degree rectangle at the current mask.scaleX and mask.scaleY.
	// If the image is perfectly square (and mask.scaleX == mask.scaleY),
	//  it would be in 90 degree intervals starting at 225 degrees with pt.0.
	sf::VertexArray maskArrayAtZeroDegrees(sf::PrimitiveType::TriangleStrip, 4);
	set_mask_array_stuff(maskArrayAtZeroDegrees, txt, maskColor, mask.x, mask.y, mask.scaleX*mask.width, mask.scaleY*mask.height, 0 /* angle */);
	float pt0Base, pt1Base, pt2Base, pt3Base; // Base angles
	sf::Vector2f midPointZeroDegrees = middle_of_four_points(maskArrayAtZeroDegrees);
	compute_angles_to_middle(maskArrayAtZeroDegrees, midPointZeroDegrees, &pt0Base, &pt1Base, &pt2Base, &pt3Base);

	// New angles
	pt0Angle = pt0Base + mask.angle;
	pt1Angle = pt1Base + mask.angle;
	pt2Angle = pt2Base + mask.angle;
	pt3Angle = pt3Base + mask.angle;

	// Set the new positions
	maskArray[0].position = sf::Vector2f(midPoint.x + pt0Radius * cos(pt0Angle), midPoint.y + pt0Radius * sin(pt0Angle));
	maskArray[1].position = sf::Vector2f(midPoint.x + pt1Radius * cos(pt1Angle), midPoint.y + pt1Radius * sin(pt1Angle));
	maskArray[2].position = sf::Vector2f(midPoint.x + pt2Radius * cos(pt2Angle), midPoint.y + pt2Radius * sin(pt2Angle));
	maskArray[3].position = sf::Vector2f(midPoint.x + pt3Radius * cos(pt3Angle), midPoint.y + pt3Radius * sin(pt3Angle));

	/* BEGIN: Debug visualization of the four angles to the midpoint */
	/*sf::RectangleShape line(sf::Vector2f(100.0f, 3.0f));
	line.setPosition(midPoint);
	int meme = rand() % 4;
	if (meme % 4 == 0) {
		line.rotate(pt0Angle / (2.0 * M_PI) * 360.0f);
	} else if (meme % 4 == 1) {
		line.rotate(pt1Angle / (2.0 * M_PI) * 360.0f);
	} else if (meme % 4 == 2) {
		line.rotate(pt2Angle / (2.0 * M_PI) * 360.0f);
	} else if (meme % 4 == 3) {
		line.rotate(pt3Angle / (2.0 * M_PI) * 360.0f);
	}*/
	/* END: Debug visualization of the four angles to the midpoint */

//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	sf::RenderStates state(&txt);
	//state.blendMode = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha);
	renderTxt.draw(maskArray, sf::RenderStates(&txt));
	//renderTxt.draw(circle);
	//renderTxt.draw(line);
	renderTxt.display();
	return renderTxt.getTexture().copyToImage();
}

void print_map(std::map<sf::Uint32, float> map) {
	for (auto const &e : map) {
		sf::Color c(e.first);
		std::cout << (int)c.r << ' ' << (int)c.g << ' ' << (int)c.b << ' ' << (int)c.a << ": " << e.second << '\n';
	}
}

float brightness_of(sf::Color color) {
	return color.g;
	// Copied from: https://stackoverflow.com/a/596243
	return (0.2126*color.r + 0.7152*color.g + 0.0722*color.b);
}

// Like a "Levels" filter in Krita, making the darks darker.
// Does not affect the alpha channel.
void apply_brightness_filter(RGBImage frame) {
	float cutoff = 0.35;

	for (size_t i = 0; i < frame.size; i += 4) {
		float brightness = brightness_of(sf::Color(frame.data[i+0], frame.data[i+1], frame.data[i+2]));
		float new_brightness = 255.0 * ((cutoff - brightness / 255.0)) / (cutoff - 1);

		// RGB, not touching alpha
		frame.data[i+0] = std::max(0.0, (frame.data[i+0] / 255.0 - cutoff) * new_brightness);
		frame.data[i+1] = std::max(0.0, (frame.data[i+1] / 255.0 - cutoff) * new_brightness);
		frame.data[i+2] = std::max(0.0, (frame.data[i+2] / 255.0 - cutoff) * new_brightness);
	}
}

void apply_bw_filter(RGBImage frame) {
	for (size_t i = 0; i < frame.size; i += 4) {
		float brightness = brightness_of(sf::Color(frame.data[i+0], frame.data[i+1], frame.data[i+2]));
		if (brightness < 105) {
			frame.data[i+0] = 0;
			frame.data[i+1] = 0;
			frame.data[i+2] = 0;
		} else {
			frame.data[i+0] = 255;
			frame.data[i+1] = 255;
			frame.data[i+2] = 255;
		}
	}
}

float detect_temperature(RGBImage frame, sf::Image *mask) {
	if (frame.width != mask->getSize().x) {
		std::cerr << "detect_temperature(): frame and mask width differs, exiting\n";
		exit(1);
	}

	if (frame.height != mask->getSize().y) {
		std::cerr << "detect_temperature(): frame and mask height differs, exiting\n";
		exit(1);
	}

	// Color (uint32), cumulative brightness (int32)
	std::map<sf::Uint32, float> segments_map;
	// Color (uint32), count of samples
	std::map<sf::Uint32, int> segments_count_map;

	const sf::Uint32 reference_point_segment = 0x6400aaff; // RGBA Color of the ominous purple colored square

	// Initialize the map
	for (size_t y = 0; y < frame.height; y++) {
		for (size_t x = 0; x < frame.width; x++) {
			sf::Color maskColor = mask->getPixel(x, y);
			if (maskColor.a == 0) {
				continue;
			}
			//segments_map[maskColor.toInteger()] = 0;
			segments_map[maskColor.toInteger()] = 0;
			segments_count_map[maskColor.toInteger()] = 0;
		}
	}

	for (size_t y = 0; y < frame.height; y++) {
		for (size_t x = 0; x < frame.width; x++) {
			sf::Color maskColor = mask->getPixel(x, y);
			if (maskColor.a == 0) {
				continue;
			}

			if (segments_map[maskColor.toInteger()] != 0) continue;
			
			size_t i = 4 * (y * WIDTH + x);
			sf::Color frameColor(frame.data[i+0], frame.data[i+1], frame.data[i+2], frame.data[i+3]);

			if (brightness_of(frameColor) == 0) {
				segments_map[maskColor.toInteger()] = 1000;
			}
			segments_count_map[maskColor.toInteger()] = 1;
			//segments_map[maskColor.toInteger()] += brightness_of(frameColor);
			//segments_count_map[maskColor.toInteger()] += 1;
		}
	}

	// Apply "brightness" score to image
	for (size_t y = 0; y < frame.height; y++) {
		for (size_t x = 0; x < frame.width; x++) {
			sf::Color maskColor = mask->getPixel(x, y);
			if (maskColor.a == 0) {
				continue;
			}
			
			float brightness_of_segment = segments_map[maskColor.toInteger()] / segments_count_map[maskColor.toInteger()];
			float compared_to_reference = segments_map[reference_point_segment] / segments_count_map[reference_point_segment];

			//sf::Uint8 b = 255 * (0.04f * std::abs(brightness_of_segment - compared_to_reference));
			//sf::Uint8 b = 255 * (256.0f * std::abs(brightness_of_segment - compared_to_reference));
			sf::Uint8 b = 255 * (brightness_of_segment / 1000.0);
			b = 255 - b;
			//sf::Uint8 b = 255 * std::max(0.0f, brightness_of_segment / float(70000)); // TODO: Softmax or something across all final segment colors
			size_t i = 4 * (y * WIDTH + x);
			frame.data[i+0] = b;
			frame.data[i+1] = b;
			frame.data[i+2] = b;
			frame.data[i+3] = b;
		}
	}

	print_map(segments_map);

	return 0.0;
}

int main() {
	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "camera", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	Webcam webcam("/dev/video0", WIDTH, HEIGHT);

	sf::Texture maskTxt;
	maskTxt.loadFromFile("mask/mask_final.png");

	bool isMaskVisible = true;
	MaskPos mask;
	mask.scaleX = 0.2;
	mask.scaleY = 0.2;
	mask.width = maskTxt.getSize().x;
	mask.height = maskTxt.getSize().y;

	const string maskFilename = "maskposition.raw";
	read_maskpos_from_file(mask, maskFilename);

	while (window.isOpen()) {
		sf::Event e;
		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed) {
				window.close();
			} else if (e.type == sf::Event::KeyPressed) {
				if (e.key.code == sf::Keyboard::Key::Space) {
					// Toggle visibility of the mask
					isMaskVisible = !isMaskVisible;
				} else if (e.key.code == sf::Keyboard::Key::Q) {
					window.close();
				}
			}
		}

		float speed = 3.0;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) speed = 0.60;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) mask.y -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) mask.x -= speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) mask.y += speed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) mask.x += speed;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) mask.angle -= speed * 0.01;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) mask.angle += speed * 0.01;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
			mask.scaleX += speed * 0.001;
			mask.scaleY += speed * 0.001;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
			mask.scaleX -= speed * 0.001;
			mask.scaleY -= speed * 0.001;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) mask.scaleX += speed * 0.001;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y)) mask.scaleY += speed * 0.001;

		// Clamp the scale
		mask.scaleX = std::max(0.01f, mask.scaleX);
		mask.scaleY = std::max(0.01f, mask.scaleY);

		window.clear();

		auto frame = webcam.frame();
		//if (false && isMaskVisible) apply_brightness_filter(frame);
		if (true) apply_bw_filter(frame);

		//sf::Image maskImg = texture_transformed_as_image(maskTxt, maskWidth, maskHeight, maskPosX, maskPosY, maskScaleX, maskScaleY, maskRotation);
		sf::Image maskImg = texture_transformed_as_image(maskTxt, mask);

		float temperature_detected = -6969.0;
		if (isMaskVisible) temperature_detected = detect_temperature(frame, &maskImg);

		sf::Texture maskImgTxt;
		maskImgTxt.loadFromImage(maskImg);

		sf::Sprite maskSpr;
		maskSpr.setTexture(maskImgTxt);
		maskSpr.setColor({80, 80, 80, 100});

		sf::Image frameImg;
		frameImg.create(WIDTH, HEIGHT, (sf::Uint8*)frame.data);
		sf::Texture txt;
		txt.loadFromImage(frameImg);
		sf::Sprite spr;
		spr.setTexture(txt);

		window.draw(spr);
		if (isMaskVisible) window.draw(maskSpr);
		window.display();
	}

	write_maskpos_to_file(mask, maskFilename);
}
