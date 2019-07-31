
#include <iostream>
#include "sgct.h"
#include "Utility.h"

#include <iostream>
#include <fstream>
#include <string>

#include "UdpSocketClient.h"
#include "webserver.h"
#include "Socket.h"

/// SGCT Engine and variables
sgct::Engine* _gEngine;
sgct::SharedBool reload_shader(false); // bool for hot-reloading shader
sgct::SharedDouble curr_time(0.0); // shared time

/// OpenGL Variables
GLuint _vao_vertex_container;
GLuint _vbo_position_buffer, _vbo_uv_buffer, _vbo_indices_buffer;
// Quad vertices (ordering is BL, TL, TR, BR)
const std::vector<GLfloat> _position_buffer_data = {
	// FLOOR with ordering
	-0.8f, -0.8f, -0.8f,  // Back Left : 0
	-0.8f, -0.8f, 0.8f,   // Front Left : 1
	0.8f, -0.8f, 0.8f,    // Front Right : 2
	0.8f, -0.8f, -0.8f,   // Back Right : 3

	// UP with ordering
	0.8f, 0.8f, -0.8f,   // Back Left : 4
	0.8f, 0.8f, 0.8f,    // Front Left : 5
	-0.8f, 0.8f, 0.8f,   // Front Right : 6
	-0.8f, 0.8f, -0.8f,  // Back Right : 7

	// LEFT
	-0.8f, -0.8f, -0.8f, // 8
	-0.8f, 0.8f, -0.8f,  // 9
	-0.8f, 0.8f, 0.8f,   // 10
	-0.8f, -0.8f, 0.8f,  // 11

	// FORWARD
	-0.8f, -0.8f, 0.8f, // 12
	-0.8f, 0.8f, 0.8f,  // 13
	0.8f, 0.8f, 0.8f,   // 14
	0.8f, -0.8f, 0.8f,  // 15

	// RIGHT
	0.8f, -0.8f, 0.8f,  // 16
	0.8f, 0.8f, 0.8f,   // 17
	0.8f, 0.8f, -0.8f,  // 18
	0.8f, -0.8f, -0.8f, // 19

	// BACK
	0.8f, -0.8f, -0.8f,  // 20
	0.8f, 0.8f, -0.8f,   // 21
	-0.8f, 0.8f, -0.8f,  // 22
	-0.8f, -0.8f, -0.8f, // 23
};
// Quad texture coords
const std::vector<GLfloat> _uv_buffer_data = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
};
// Quad indices coords
const std::vector<GLuint> _indices_buffer_data = {
	0,3,1, 3,2,1,        // Quad FLOOR
	4,7,5, 7,6,5,        // Quad UP
	8,11,9, 11,10,9,     // Quad LEFT
	12,15,13, 15,14,13,  // Quad FORWARD
	16,19,17, 19,18,17,  // Quad RIGHT
	20,13,21, 23,22,21   // Quad BACK
};

/// Server Info
const int BUFFER_LENGTH = 44;
const int REGISTER_PORT = 5000;
const int TRACKING_PORT = 5001;
std::string UNITY_CLIENT = ""; // Address of Unity client
bool UPDATE_IP = false; // Does the IP of the UDP client need to be updated?
webserver* initServer = nullptr; // TCP server, used ONLY for initialization of connection between SGCT and Unity
UdpSocketClient* udpServer = nullptr; // Server in charge of streaming between Unity and SGCT

/// Containers for textures from Unity
const int TEXTURE_BYTES_LENGTH = 7896 * 4; // 64 * 64 * 3 * 4; //960 * 1080 * 3 * 4; // size of screen capture, channels, num_images
unsigned char* rawTextureBytes = nullptr;
unsigned char* bytes_img_0 = nullptr;
unsigned char* bytes_img_1 = nullptr;
unsigned char* bytes_img_2 = nullptr;
unsigned char* bytes_img_3 = nullptr;

/// Files for textures
//const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/square.png";
//const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/test2.png";
const std::string _texture_path = "../../../../src/apps/IVLAB_Unity/images/water.png";

/// Helper Functions
void init(); // OpenGL Initialization
void mainLoop(); // Drawing funciton
void keyCallback(int key, int action); // Allow user keyboard input
void shaderReload(); // Allow shader reload on press of "r"
void myPreSyncFun(); // Sync data between nodes
void myEncodeFun(); // Used for synchorization between nodes
void myDecodeFun(); // Used for synchorization between nodes
void requestHandler(webserver::http_request* r); // Registers Unity Client with this program




int main(int argc, char* argv[])
{
	/****** * * * * * SGCT Initialization * * * * * *****/

	_gEngine = new sgct::Engine(argc, argv);

	//Bind your draw function to the render loop
	_gEngine->setInitOGLFunction(init);
	_gEngine->setDrawFunction(mainLoop);
	_gEngine->setPostSyncPreDrawFunction(shaderReload);
	_gEngine->setKeyboardCallbackFunction(keyCallback);
	_gEngine->setPreSyncFunction(myPreSyncFun);
	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

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

	// Clean up server
	delete udpServer;
	delete initServer;

	exit(EXIT_SUCCESS);
}

void init() {

	/***** * * * * * SERVER SETUP * * * * * ******/
	{
		// Wait for connection if it becomes unregistered
		initServer = new webserver(REGISTER_PORT, requestHandler);

		// Assuming we only have 1 client which will never exit our server
		std::cout << "Waiting for Unity client connection (HTTP, port " << REGISTER_PORT << ")..." << std::endl;
		while (UNITY_CLIENT.empty()) {
			initServer->oneFrame();
			Sleep(500);
		}
		std::cout << "Registered Unity Client: " << UNITY_CLIENT << std::endl;

		udpServer = new UdpSocketClient(UNITY_CLIENT.c_str(), TRACKING_PORT);
		UPDATE_IP = false;
		std::cout << "Starting UDP server on port " << TRACKING_PORT << "..." << std::endl;
	}


	/***** * * * * * OPENGL SETUP * * * * * ******/
	{
		/***** * * * * * SHADER AND VAO (GEOMETRY OBJECT) * * * * * *****/
		{
			// Set up our shader program
			sgct::ShaderManager::instance()->addShaderProgram("quad_shader",
				"../../../../src/apps/IVLAB_Unity/src/shaders/quad.vert",
				"../../../../src/apps/IVLAB_Unity/src/shaders/quad.frag");

			sgct::ShaderManager::instance()->bindShaderProgram("quad_shader");
		}



		/***** * * * * * VAO (GEOMETRY OBJECT) AND VBOS (INFO PER VERTEX) * * * * * *****/
		{
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
		}



		/***** * * * * * TEXTURES * * * * * *****/
		{
			sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
			sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);

			glActiveTexture(GL_TEXTURE0);
			sgct::TextureManager::instance()->loadTexture("quad_texture0", _texture_path, true);
			glActiveTexture(GL_TEXTURE1);
			sgct::TextureManager::instance()->loadTexture("quad_texture1", _texture_path, true);
			glActiveTexture(GL_TEXTURE2);
			sgct::TextureManager::instance()->loadTexture("quad_texture2", _texture_path, true);
			glActiveTexture(GL_TEXTURE3);
			sgct::TextureManager::instance()->loadTexture("quad_texture3", _texture_path, true);
			glActiveTexture(GL_TEXTURE4);
			sgct::TextureManager::instance()->loadTexture("quad_texture4", _texture_path, true);
			glActiveTexture(GL_TEXTURE5);
			sgct::TextureManager::instance()->loadTexture("quad_texture5", _texture_path, true);

			// Render settings for Alpha in images
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Buffer setup
			rawTextureBytes = new unsigned char[TEXTURE_BYTES_LENGTH];
			bytes_img_0 = new unsigned char[TEXTURE_BYTES_LENGTH / 4];
			bytes_img_1 = new unsigned char[TEXTURE_BYTES_LENGTH / 4];
			bytes_img_2 = new unsigned char[TEXTURE_BYTES_LENGTH / 4];
			bytes_img_3 = new unsigned char[TEXTURE_BYTES_LENGTH / 4];
		}



		/***** * * * * * CLEANUP * * * * * *****/
		sgct::ShaderManager::instance()->unBindShaderProgram();
	}
}

void mainLoop() {
	
	/***** * * * * * SERVER * * * * * ******/
	// Assuming nothing un-registers
	{

		// Make dummy matrix to send over
		float* matrix = new float[64] {
				1.0, 0.0, 0.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0,

				1.0, 0.0, 0.0, 1.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0,

				1.0, 0.0, 0.0, 2.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0,

				1.0, 0.0, 0.0, 3.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0
		};
		int length = sizeof(float) * 64;
		unsigned char* bytes = reinterpret_cast<unsigned char*>(matrix);

		// If no error, do something
		if (!udpServer->sendMessage(reinterpret_cast<char*>(bytes), length)) {
			delete[] bytes;
		}

		// If no error, move our raw bytes into individual arrays and then update textures
		if (!udpServer->receiveMessage(reinterpret_cast<char*>(rawTextureBytes), TEXTURE_BYTES_LENGTH)) {
			// Copy memory over
			bytes_img_0 = &rawTextureBytes[0];
			bytes_img_1 = &rawTextureBytes[TEXTURE_BYTES_LENGTH / 4];
			bytes_img_2 = &rawTextureBytes[TEXTURE_BYTES_LENGTH / 2];
			bytes_img_3 = &rawTextureBytes[(int)(TEXTURE_BYTES_LENGTH * 3.0f / 4.0f)];

			// update textures
			sgct_core::Image* img = new sgct_core::Image();

			glActiveTexture(GL_TEXTURE0);
			img->loadJPEG(bytes_img_0, TEXTURE_BYTES_LENGTH / 4);
			sgct::TextureManager::instance()->loadTexture("quad_texture0", img, true);
			glActiveTexture(GL_TEXTURE1);
			img->loadJPEG(bytes_img_1, TEXTURE_BYTES_LENGTH / 4);
			sgct::TextureManager::instance()->loadTexture("quad_texture1", img, true);
			glActiveTexture(GL_TEXTURE2);
			img->loadJPEG(bytes_img_2, TEXTURE_BYTES_LENGTH / 4);
			sgct::TextureManager::instance()->loadTexture("quad_texture2", img, true);
			glActiveTexture(GL_TEXTURE3);
			img->loadJPEG(bytes_img_3, TEXTURE_BYTES_LENGTH / 4);
			sgct::TextureManager::instance()->loadTexture("quad_texture3", img, true);

			delete img;
		}
	}

	/***** * * * * * OPENGL * * * * ******/ 
	{
		// Clear the screen
		glClearColor(0, 0, 0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Enable properties
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);


		// Bind our shader and vao
		sgct::ShaderManager::instance()->bindShaderProgram("quad_shader");
		glBindVertexArray(_vao_vertex_container);

		// PVM Matrix setup and assignment
		glm::mat4 scene_mat = glm::perspectiveFov(90.0f, 16.0f, 9.0f, 0.0f, 100.0f);
		glm::mat4 pvm = _gEngine->getCurrentModelViewProjectionMatrix() * scene_mat;
		glUniformMatrix4fv(
			sgct::ShaderManager::instance()->getShaderProgram("quad_shader").getUniformLocation("pvm"),
			1,
			GL_FALSE,
			glm::value_ptr(pvm));

		// Draw our 6 quads one buy one
		for (int i = 0; i < 6; i++) {

			// Bind our texture uniform
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("quad_texture" + std::to_string(i)));
			glUniform1i(sgct::ShaderManager::instance()->getShaderProgram("quad_shader").getUniformLocation("tex"), 0);

			// Bind our geometry object and indices
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices_buffer);
			glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, i * 4);

		}

		// Cleanup
		glBindVertexArray(GL_FALSE);
		sgct::ShaderManager::instance()->unBindShaderProgram();
		// Disable properties
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}
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

void myPreSyncFun() {
	//set the time only on the master
	if (_gEngine->isMaster()) {
		//get the time in seconds
		curr_time.setVal(sgct::Engine::getTime());
	}
}

void myEncodeFun() {
	sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun() {
	sgct::SharedData::instance()->readDouble(&curr_time);
}

void requestHandler(webserver::http_request* r) {
	Socket s = *(r->s_);
	if (r->path_ == "/") {
		std::string title = "SGCT UDP Streaming Server";
		std::string body = "<h1>" + title + "</h1>";

		r->answer_ = "<html><head><title>";
		r->answer_ += title;
		r->answer_ += "</title></head><body>";
		r->answer_ += body;
		r->answer_ += "</body></html>";
	} else if (r->path_ == "/register") {
		UNITY_CLIENT = r->params_["ip"];
		r->answer_ = "Registered";
		UPDATE_IP = true;
	} else if (r->path_ == "/unregister") {
		UNITY_CLIENT = "";
		r->answer_ = "Unregistered";
	} else {
		r->status_ = "404 Not Found";
		r->answer_ = "Wrong URL";
	}
}