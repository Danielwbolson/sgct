
#include "sgct.h"
#include "Utility.h"

sgct::Engine* _gEngine;

GLuint _vao_vertex_container;
GLuint _vbo_position_buffer, _vbo_uv_buffer, _vbo_indices_buffer;
GLuint _shader_program;
GLuint _texture;

sgct_core::Image* _img;
//const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/test2.png";
const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/water.png";

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
sgct::SharedBool reload_shader(false); // bool for hot-reloading shader



void init(); // OpenGL Initialization
void drawQuad(); // Drawing funciton
void keyCallback(int key, int action); // Allow user keyboard input
void shaderReload(); // Allow shader reload on press of "r"

int main(int argc, char* argv[])
{
	/****** * * * * * SGCT Initialization * * * * * *****/

	_gEngine = new sgct::Engine(argc, argv);

	//Bind your draw function to the render loop
	_gEngine->setInitOGLFunction(init);
	_gEngine->setDrawFunction(drawQuad);
	_gEngine->setPostSyncPreDrawFunction(shaderReload);
	_gEngine->setKeyboardCallbackFunction(keyCallback);

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
	glUseProgram(_shader_program);



	/***** * * * * * VAO (GEOMETRY OBJECT) AND VBOS (INFO PER VERTEX) * * * * * *****/
	// Generate and bind our vao, our geometry container that holds vbos
	glGenVertexArrays(1, &_vao_vertex_container);
	glBindVertexArray(_vao_vertex_container);

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

	// Unbind for good habit
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
	glBindVertexArray(GL_FALSE);



	/***** * * * * * TEXTURE * * * * * *****/
	// Load our image
	_img = new sgct_core::Image();
	bool loaded = _img->load(_texture_path);

	// Generate opengl reference to texture
	glGenTextures(1, &_texture);
	glBindTexture(GL_TEXTURE_2D, _texture);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Find out texture variables (Taken from TextureManager.cpp)
	// Find out if we are rgb or bgr and number of channels
	unsigned int bpc = static_cast<unsigned int>(_img->getBytesPerChannel());

	bool isBGR = _img->getPreferBGRImport();
	GLint textureType = isBGR ? GL_BGR : GL_RGB;
	GLint internalFormat = (bpc == 1 ? GL_RGB8 : GL_RGB16);
	GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);

	if (_img->getChannels() == 4) {
		textureType = isBGR ? GL_BGRA : GL_RGBA;
		internalFormat = (bpc == 1 ? GL_RGBA8 : GL_RGBA16);
	}
	else if (_img->getChannels() == 1) {
		textureType = GL_LUMINANCE;
		internalFormat = (bpc == 1 ? GL_RG8 : GL_RG16);
	}
	else if (_img->getChannels() == 2) {
		textureType = GL_LUMINANCE_ALPHA;
		internalFormat = (bpc == 1 ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE16_ALPHA16);
	}

	// Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(_img->getWidth()), static_cast<GLsizei>(_img->getHeight()), 0, textureType, format, _img->getData());

	// Texture paramters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Render settings for Alpha in images
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	/***** * * * * * CLEANUP * * * * * *****/
	// Let go of our vertex array and texture until we need it again, avoid changing it on accident
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(GL_FALSE);
}

// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// https://github.com/Danielwbolson/CppEngine/blob/master/CppEngine/src/MeshRendererSystem.cpp

void drawQuad() {
	// Clear the screen
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Enable properties
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

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


	// Cleanup
	glBindVertexArray(GL_FALSE);
	glUseProgram(GL_FALSE);
	// Disable properties
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void keyCallback(int key, int action) {
	if (_gEngine->isMaster()) {
		switch (key) {
		case SGCT_KEY_R:
			if (action == SGCT_PRESS)
				reload_shader.setVal(true);
			break;
		}
	}
}

void shaderReload() {
	if (reload_shader.getVal()) {
		reload_shader.setVal(false); //reset

		sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram("quad_shader");
		sp.reload();
	}
}