
#include "sgct.h"
#include "Utility.h"

sgct::Engine* _gEngine;

GLuint _vao_vertex_container;
GLuint _vbo_position_buffer, _vbo_uv_buffer, _vbo_indices_buffer;
GLuint _shader_program;
GLuint _texture;

sgct_core::Image* _img;
const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/lion_king.png";
//const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/test2.png";

// Quad vertices
const std::vector<GLfloat> _position_buffer_data = {
	-0.8f, -0.8f, 0.0f, // Bottom Left : 0
	-0.8f, 0.8f, 0.0f,  // Top Left : 1
	0.8f, 0.8f, 0.0f,   // Top Right : 2
	0.8f, -0.8f, 0.0f   // Bottom Right : 3
};

// Quad texture coords
const std::vector<GLfloat> _uv_buffer_data = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

// Quad indices coords
const std::vector<GLuint> _indices_buffer_data = {
	0,3,1,
	3,2,1
};

void init();
void drawQuad();

int main(int argc, char* argv[])
{
	/****** * * * * * SGCT Initialization * * * * * *****/

	_gEngine = new sgct::Engine(argc, argv);

	//Bind your draw function to the render loop
	_gEngine->setInitOGLFunction(init);
	_gEngine->setDrawFunction(drawQuad);

	if (!_gEngine->init())
	{
		delete _gEngine;
		return EXIT_FAILURE;
	}

	/****** * * * * * END SGCT Initialization * * * * * *****/


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

	// Main loop
	_gEngine->render();

	// Clean up engine, opengl and exit
	delete _gEngine;
	delete _img;
	glDeleteProgram(_shader_program);
	glDeleteBuffers(1, &_vbo_indices_buffer);
	glDeleteBuffers(1, &_vbo_position_buffer);
	glDeleteBuffers(1, &_vbo_uv_buffer);
	glDeleteVertexArrays(1, &_vao_vertex_container);
	glDeleteTextures(1, &_texture);

	exit(EXIT_SUCCESS);
}

// Initializes our quad and tex coordinates
void init() {

	/***** * * * * * SHADER AND VAO (GEOMETRY OBJECT) * * * * * *****/
	// Set up our shader program
	_shader_program = util::initShaderFromFiles("quad.vert", "quad.frag");
	glEnable(GL_DEPTH_TEST);

	// Generate and bind our vao, our geometry container that holds vbos
	glGenVertexArrays(1, &_vao_vertex_container);
	glBindVertexArray(_vao_vertex_container);


	/***** * * * * * TEXTURE * * * * * *****/
	// Load our image
	_img = new sgct_core::Image();
	bool loaded = _img->loadPNG(_texture_path);

	// Generate opengl reference to texture
	glGenTextures(1, &_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(_img->getWidth()), static_cast<GLsizei>(_img->getHeight()), 0, GL_BGRA, GL_UNSIGNED_BYTE, _img->getData());

	// Texture paramters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	// Render settings for Alpha in images
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	/***** * * * * * VBOS (INFO PER VERTEX) * * * * * *****/
	// Generate our positionbuffer
	glGenBuffers(1, &_vbo_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_position_buffer); // Bind our vertexBuffer to be our current buffer
	glBufferData(GL_ARRAY_BUFFER, _position_buffer_data.size() * sizeof(GL_FLOAT), &(_position_buffer_data[0]), GL_STATIC_DRAW); // Assign vertex data
	// Connect our positionbuffer to our position input for our shader
	GLint positionAttrib = glGetAttribLocation(_shader_program, "inPosition");
	glEnableVertexAttribArray(positionAttrib);
	glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//Attribute, vals per attrib, type, isNormalized, stride, offset

	// UV buffer
	glGenBuffers(1, &_vbo_uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, _uv_buffer_data.size() * sizeof(GL_FLOAT), &(_uv_buffer_data[0]), GL_STATIC_DRAW);
	GLint uvAttrib = glGetAttribLocation(_shader_program, "inUV");
	glEnableVertexAttribArray(uvAttrib);
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Indices buffer
	glGenBuffers(1, &_vbo_indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices_buffer_data.size() * sizeof(GL_UNSIGNED_INT), &(_indices_buffer_data[0]), GL_STATIC_DRAW);



	/***** * * * * * CLEANUP * * * * * *****/
	// Let go of our vertex array and texture until we need it again, avoid changing it on accident
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// https://github.com/Danielwbolson/CppEngine/blob/master/CppEngine/src/MeshRendererSystem.cpp

void drawQuad() {
	// Clear the screen
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind our shader and vao
	glUseProgram(_shader_program);
	glBindVertexArray(_vao_vertex_container);

	// Bind our texture uniform
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _texture);
	glUniform1i(glGetUniformLocation(_shader_program, "tex"), 0);

	// Bind our geometry object and indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices_buffer);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices_buffer_data.size()), GL_UNSIGNED_INT, 0);
}
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/
