
#include "sgct.h"
#include "Utility.h"

sgct::Engine* gEngine;

GLuint vao_vertexContainer;
GLuint vbo_positionBuffer, vbo_textureBuffer, vbo_indices_buffer;
GLuint shaderProgram;

// Quad vertices
std::vector<GLfloat> position_buffer_data = {
	-0.8f, 0.8f, 0.0f,  // Top Left
	-0.8f, -0.8f, 0.0f, // Bottom Left
	0.8f, 0.8f, 0.0f,   // Top Right
	0.8f, -0.8f, 0.0f   // Bottom Right
};

// Quad texture coords
std::vector<GLfloat> texture_buffer_data = {
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

// Quad indices coords
std::vector<GLuint> indices_buffer_data = {
	0,1,2,
	1,3,2
};

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

	/* Potential code for Unity
	// Getting proj and view matrices for each window/viewport pairing
	for (unsigned int i = 0; i < gEngine->getNumberOfWindows(); i++) {
		sgct::SGCTWindow* window = gEngine->getWindowPtr(i);

		for (unsigned int j = 0; j < window->getNumberOfViewports(); j++) {
			sgct_core::BaseViewport* viewport = window->getViewport(j);

			sgct_core::SGCTProjection* projection =  viewport->getProjection();

			glm::mat4 projMatrix = projection->getProjectionMatrix();
			glm::mat4 viewMatrix = projection->getViewMatrix();

			/// SEND TO UNITY
		}
	}
	*/

	//Init geometry, buffers, shader
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
	glEnable(GL_DEPTH_TEST);



	// Generate and bind our vao, our geometry container that holds vbos
	glGenVertexArrays(1, &vao_vertexContainer);
	glBindVertexArray(vao_vertexContainer);



	// Generate our positionbuffer
	glGenBuffers(1, &vbo_positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_positionBuffer); // Bind our vertexBuffer to be our current buffer
	glBufferData(GL_ARRAY_BUFFER, position_buffer_data.size() * sizeof(GL_FLOAT), &(position_buffer_data[0]), GL_STATIC_DRAW); // Assign vertex data
																				// Do this way to get pointer to vector location (worked in engine)
	// Connect our positionbuffer to our position input for our vert shader
	GLint positionAttrib = glGetAttribLocation(shaderProgram, "inPosition");
	glEnableVertexAttribArray(positionAttrib);
	glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//Attribute, vals per attrib, type, isNormalized, stride, offset



	// Generate our texturebuffer
	glGenBuffers(1, &vbo_textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, texture_buffer_data.size() * sizeof(GL_FLOAT), &(texture_buffer_data[0]), GL_STATIC_DRAW);

	// Connect our texturebuffer to our texture input for our vert shader
	GLint textureAttrib = glGetAttribLocation(shaderProgram, "inTexcoords");
	glEnableVertexAttribArray(textureAttrib);
	glVertexAttribPointer(textureAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//Attribute, vals per attrib, type, isNormalized, stride, offset



	// Generate our indicesbuffer
	glGenBuffers(1, &vbo_indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_data.size() * sizeof(GL_UNSIGNED_INT), &(indices_buffer_data[0]), GL_STATIC_DRAW);

	// Let go of our vertex array until we need it again, avoid changing it on accident
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
	glDrawElements(GL_TRIANGLES, indices_buffer_data.size(), GL_UNSIGNED_INT, 0);
}
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/