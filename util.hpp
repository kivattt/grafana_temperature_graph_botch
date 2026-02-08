#ifndef UTIL_HPP
#define UTIL_HPP

#include <math.h>

float hypot(sf::Vector2f a, sf::Vector2f b) {
	float x = a.x - b.x;
	float y = a.y - b.y;
	return sqrt(x*x + y*y);
}

#endif // UTIL_HPP
