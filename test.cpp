#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>

#include "util.hpp"

struct HypotTestCase {
	HypotTestCase(sf::Vector2f a, sf::Vector2f b, float expected) : a(a), b(b), expected(expected) {}

	sf::Vector2f a;
	sf::Vector2f b;
	float expected;
};

int main() {
	std::vector<HypotTestCase> testCases = {
		HypotTestCase(
			{0, 0},
			{4, 3},
			5.0
		),
		HypotTestCase(
			{10, 10},
			{14, 13},
			5.0
		),
		HypotTestCase(
			{-10, -10},
			{-14, -13},
			5.0
		),
		HypotTestCase(
			{-0, -0},
			{-4, -3},
			5.0
		)
	};

	for (HypotTestCase &test : testCases) {
		float got = hypot(test.a, test.b);
		if (got != test.expected) {
		std::cerr << "Test failed, expected " << test.expected << ", but got: " << got << '\n';
		return 1;
		}
	}
}
