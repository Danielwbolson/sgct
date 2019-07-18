
#include "sgct.h"
#include "Utility.h"

sgct::Engine* _gEngine;

GLuint _vao_vertex_container;
GLuint _vbo_position_buffer, _vbo_uv_buffer, _vbo_indices_buffer;

//const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/lion_king.png";
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

void init(); // OpenGL Initialization
void drawQuad(); // Drawing funciton
void keyCallback(int key, int action); // Allow user keyboard input
void shaderReload(); // Allow shader reload on press of "r"

sgct::SharedBool reload_shader(false); // bool for hot-reloading shader

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
	glDeleteBuffers(1, &_vbo_indices_buffer);
	glDeleteBuffers(1, &_vbo_position_buffer);
	glDeleteBuffers(1, &_vbo_uv_buffer);
	glDeleteVertexArrays(1, &_vao_vertex_container);

	exit(EXIT_SUCCESS);
}

// Initializes our quad and tex coordinates
void init() {

	/***** * * * * * SHADER AND VAO (GEOMETRY OBJECT) * * * * * *****/
	// Set up our shader program
	sgct::ShaderManager::instance()->addShaderProgram("quad_shader",
		"../../../../src/apps/IVLAB_Unity/quad.vert",
		"../../../../src/apps/IVLAB_Unity/quad.frag");

	sgct::ShaderManager::instance()->bindShaderProgram("quad_shader");



	/***** * * * * * VAO (GEOMETRY OBJECT) AND VBOS (INFO PER VERTEX) * * * * * *****/
	// Generate and bind our vao, our geometry container that holds vbos
	glGenVertexArrays(1, &_vao_vertex_container);
	glBindVertexArray(_vao_vertex_container);


	// Generate our positionbuffer
	glGenBuffers(1, &_vbo_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_position_buffer); // Bind our vertexBuffer to be our current buffer
	glBufferData(GL_ARRAY_BUFFER, _position_buffer_data.size() * sizeof(GL_FLOAT), &(_position_buffer_data[0]), GL_STATIC_DRAW); // Assign vertex data
	// Connect our positionbuffer to our position input for our shader
	GLint positionAttrib = sgct::ShaderManager::instance()->getShaderProgram("quad_shader").getAttribLocation("inPosition");
	glEnableVertexAttribArray(positionAttrib);
	glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//Attribute, vals per attrib, type, isNormalized, stride, offset

	// UV buffer
	glGenBuffers(1, &_vbo_uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, _uv_buffer_data.size() * sizeof(GL_FLOAT), &(_uv_buffer_data[0]), GL_STATIC_DRAW);
	GLint uvAttrib = sgct::ShaderManager::instance()->getShaderProgram("quad_shader").getAttribLocation("inUV");
	glEnableVertexAttribArray(uvAttrib);
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Indices buffer
	glGenBuffers(1, &_vbo_indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices_buffer_data.size() * sizeof(GL_UNSIGNED_INT), &(_indices_buffer_data[0]), GL_STATIC_DRAW);

	// unbind to to defaults in order to avoid any accidental changes
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
	glBindVertexArray(GL_FALSE);



	/***** * * * * * TEXTURE * * * * * *****/
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::instance()->loadTexture("quad_texture", _texture_path, true);

	// Render settings for Alpha in images
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	/***** * * * * * CLEANUP * * * * * *****/
	sgct::ShaderManager::instance()->unBindShaderProgram();
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
	sgct::ShaderManager::instance()->bindShaderProgram("quad_shader");
	glBindVertexArray(_vao_vertex_container);

	// Bind our texture uniform
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("quad_texture"));
	glUniform1i(sgct::ShaderManager::instance()->getShaderProgram("quad_shader").getUniformLocation("tex"), 0);

	// Bind our geometry object and indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices_buffer);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices_buffer_data.size()), GL_UNSIGNED_INT, 0);


	// Cleanup
	glBindVertexArray(GL_FALSE);
	sgct::ShaderManager::instance()->unBindShaderProgram();
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