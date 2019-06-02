#include "Renderer.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

int main() {
	Renderer app(WIDTH, HEIGHT, MAX_FRAMES_IN_FLIGHT);

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}