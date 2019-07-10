
#include "sgct.h"
#include "Utility.h"

sgct::Engine * gEngine;

GLuint vao_vertexContainer;
GLuint vbo_vertexBuffer, vbo_textureBuffer, vbo_indices_buffer;
GLuint shaderProgram;

// Quad vertices
const GLfloat g_vertex_buffer_data[] = {
	-0.8f, 0.8f, 0.0f,
	-0.8f, -0.8f, 0.0f,
	0.8f, 0.8f, 0.0f,
	0.8f, -0.8f, 0.0f
};

// Quad texture coords
const GLfloat g_texture_buffer_data[] = {
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

// Quad indices coords
const GLuint g_indices_buffer_data[] = {
	1,
	2,
	3,
	4
}; \

void Init();
void drawQuad();

int main(int argc, char* argv[])
{
	gEngine = new sgct::Engine(argc, argv);

	if (!gEngine->init())
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	//Bind your draw function to the render loop
	gEngine->setDrawFunction(drawQuad);

	//// Getting proj and view matrices for each window/viewport pairing
	//for (unsigned int i = 0; i < gEngine->getNumberOfWindows(); i++) {
	//	sgct::SGCTWindow* window = gEngine->getWindowPtr(i);

	//	for (unsigned int j = 0; j < window->getNumberOfViewports(); j++) {
	//		sgct_core::BaseViewport* viewport = window->getViewport(j);

	//		sgct_core::SGCTProjection* projection =  viewport->getProjection();

	//		glm::mat4 projMatrix = projection->getProjectionMatrix();
	//		glm::mat4 viewMatrix = projection->getViewMatrix();

	//		/// SEND TO UNITY
	//	}
	//}

	//Init Quad
	Init();

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit(EXIT_SUCCESS);
}

// Initializes our quad and tex coordinates
void Init() {
	// Set up our shader program
	shaderProgram = util::initShaderFromFiles("quad.vert", "quad.frag");


	// Generate and bind our vao, our geometry container that holds vbos
	glGenVertexArrays(1, &vao_vertexContainer);
	glBindVertexArray(vao_vertexContainer);

	// Generate our vertexbuffer
	glGenBuffers(1, &vbo_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexBuffer); // Bind our vertexBuffer to be our current buffer
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(g_vertex_buffer_data) * sizeof(float), g_vertex_buffer_data, GL_STATIC_DRAW); // Assign vertex data

	// Generate our texturebuffer
	glGenBuffers(1, &vbo_textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(g_texture_buffer_data) * sizeof(float), g_texture_buffer_data, GL_STATIC_DRAW);

	// Generate our indicesbuffer
	glGenBuffers(1, &vbo_indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_indices_buffer_data) * sizeof(GL_UNSIGNED_INT), g_indices_buffer_data, GL_STATIC_DRAW);

	glBindVertexArray(0);

}

// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// https://github.com/Danielwbolson/CppEngine/blob/master/CppEngine/src/MeshRendererSystem.cpp

void drawQuad() {
	// Clear the screen
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// Bind our shader and vao
	glUseProgram(shaderProgram);
	glBindVertexArray(vao_vertexContainer);

	// Bind our indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices_buffer);
	glDrawElements(GL_TRIANGLE_STRIP, sizeof(g_indices_buffer_data), GL_UNSIGNED_INT, 0);
}