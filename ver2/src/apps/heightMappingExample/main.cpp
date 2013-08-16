#include <stdlib.h>
#include <stdio.h>

#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

void keyCallback(int key, int action);
void drawTerrainGrid( float width, float height, unsigned int wRes, unsigned int dRes );

size_t myTextureHandles[2];
int myTextureLocations[2];
int curr_timeLoc;
bool mPause = false;
GLuint myTerrainDisplayList = 0;

//light data
GLfloat lightPosition[] = { -2.0f, 5.0f, 5.0f, 1.0f };
GLfloat lightAmbient[]= { 0.1f, 0.1f, 0.1f, 1.0f };
GLfloat lightDiffuse[]= { 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat lightSpecular[]= { 1.0f, 1.0f, 1.0f, 1.0f };

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool wireframe(false);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);
sgct::SharedBool useTracking(false);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::Instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::Instance()->setDecodeFunction( myDecodeFun );

	// Main loop
	gEngine->render();

	// Clean up
	glDeleteLists(myTerrainDisplayList, 1);
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{	
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	
	glTranslatef( 0.0f, -0.15f, 2.5f );
	glRotatef( static_cast<float>( curr_time.getVal() ) * 8.0f, 0.0f, 1.0f, 0.0f );

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle( myTextureHandles[0] ));
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle( myTextureHandles[1] ));
	glEnable(GL_TEXTURE_2D);

	//set current shader program
	sgct::ShaderManager::Instance()->bindShader( "Heightmap" );
	glUniform1f( curr_timeLoc, static_cast<float>( curr_time.getVal() ) );

	glLineWidth(2.0); //for wireframe
	glCallList(myTerrainDisplayList);

	//unset current shader program
	sgct::ShaderManager::Instance()->unBindShader();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

void myPreSyncFun()
{
	if( gEngine->isMaster() && !mPause)
	{
		curr_time.setVal( curr_time.getVal() + gEngine->getAvgDt());
	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setWireframe(wireframe.getVal());
	gEngine->setDisplayInfoVisibility(info.getVal());
	gEngine->setStatsGraphVisibility(stats.getVal());
	sgct_core::ClusterManager::Instance()->getTrackingManagerPtr()->setEnabled( useTracking.getVal() );

	if( takeScreenshot.getVal() )
	{
		gEngine->takeScreenshot();
		takeScreenshot.setVal(false);
	}
}

void myInitOGLFun()
{
	glEnable( GL_DEPTH_TEST );
	//glDepthMask( GL_TRUE );
	//glDisable( GL_CULL_FACE );
	glEnable( GL_NORMALIZE );
	glEnable( GL_COLOR_MATERIAL );
	glShadeModel( GL_SMOOTH );
	glEnable( GL_LIGHTING );

	//Set up light 0
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	//create and compile display list
	myTerrainDisplayList = glGenLists(1);
	glNewList(myTerrainDisplayList, GL_COMPILE);
	//draw the terrain once to add it to the display list
	drawTerrainGrid( 1.0f, 1.0f, 256, 256 );
	glEndList();

	//sgct::TextureManager::Instance()->setAnisotropicFilterSize(4.0f);
	sgct::TextureManager::Instance()->loadTexure(myTextureHandles[0], "heightmap", "heightmap.png", true, 0);
	sgct::TextureManager::Instance()->loadTexure(myTextureHandles[1], "normalmap", "normalmap.png", true, 0);

	sgct::ShaderManager::Instance()->addShader( "Heightmap", "heightmap.vert", "heightmap.frag" );

	sgct::ShaderManager::Instance()->bindShader( "Heightmap" );
	myTextureLocations[0] = -1;
	myTextureLocations[1] = -1;
	curr_timeLoc = -1;
	myTextureLocations[0] = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "hTex" );
	myTextureLocations[1] = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "nTex" );
	curr_timeLoc = sgct::ShaderManager::Instance()->getShader( "Heightmap").getUniformLocation( "curr_time" );

	glUniform1i( myTextureLocations[0], 0 );
	glUniform1i( myTextureLocations[1], 1 );
	sgct::ShaderManager::Instance()->unBindShader();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( &curr_time );
	sgct::SharedData::Instance()->writeBool( &wireframe );
	sgct::SharedData::Instance()->writeBool( &info );
	sgct::SharedData::Instance()->writeBool( &stats );
	sgct::SharedData::Instance()->writeBool( &takeScreenshot );
	sgct::SharedData::Instance()->writeBool( &useTracking );
}

void myDecodeFun()
{
	sgct::SharedData::Instance()->readDouble( &curr_time );
	sgct::SharedData::Instance()->readBool( &wireframe );
	sgct::SharedData::Instance()->readBool( &info );
	sgct::SharedData::Instance()->readBool( &stats );
	sgct::SharedData::Instance()->readBool( &takeScreenshot );
	sgct::SharedData::Instance()->readBool( &useTracking );
}

/*!
Will draw a flat surface that can be used for the heightmapped terrain.
@param	width	Width of the surface
@param	depth	Depth of the surface
@param	wRes	Width resolution of the surface
@param	dRes	Depth resolution of the surface
*/
void drawTerrainGrid( float width, float depth, unsigned int wRes, unsigned int dRes )
{
	float wStart = -width * 0.5f;
	float dStart = -depth * 0.5f;

	float dW = width / static_cast<float>( wRes );
	float dD = depth / static_cast<float>( dRes );

	for( unsigned int depthIndex = 0; depthIndex < dRes; ++depthIndex )
    {
		float dPosLow = dStart + dD * static_cast<float>( depthIndex );
		float dPosHigh = dStart + dD * static_cast<float>( depthIndex + 1 );
		float dTexCoordLow = depthIndex / static_cast<float>( dRes );
		float dTexCoordHigh = (depthIndex+1) / static_cast<float>( dRes );

		glBegin( GL_TRIANGLE_STRIP );
		glNormal3f(0.0f,1.0f,0.0);
        for( unsigned widthIndex = 0; widthIndex < wRes; ++widthIndex )
        {
			float wPos = wStart + dW * static_cast<float>( widthIndex );
			float wTexCoord = widthIndex / static_cast<float>( wRes );

			glMultiTexCoord2f(GL_TEXTURE0, wTexCoord, dTexCoordLow);
			glMultiTexCoord2f(GL_TEXTURE1, wTexCoord, dTexCoordLow);
			glVertex3f( wPos, 0.0f, dPosLow );

			glMultiTexCoord2f(GL_TEXTURE0, wTexCoord, dTexCoordHigh);
			glMultiTexCoord2f(GL_TEXTURE1, wTexCoord, dTexCoordHigh);
			glVertex3f( wPos, 0.0f, dPosHigh );
        }

		glEnd();
    }
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'S':
			if(action == SGCT_PRESS)
				stats.toggle();
			break;

		case 'I':
			if(action == SGCT_PRESS)
				info.toggle();
			break;

		case 'W':
			if(action == SGCT_PRESS)
				wireframe.toggle();
			break;

		case 'Q':
			if(action == SGCT_PRESS)
				gEngine->terminate();
			break;

		case 'T':
			if(action == SGCT_PRESS)
				useTracking.toggle();
			break;

		case 'E':
			if(action == SGCT_PRESS)
			{
				glm::dmat4 xform = glm::translate( glm::dmat4(1.0), glm::dvec3(0.0f, 0.0f, 4.0f) );
				sgct_core::ClusterManager::Instance()->getUserPtr()->setTransform(xform);
			}
			break;

		case SGCT_KEY_SPACE:
			if(action == SGCT_PRESS)
				mPause = !mPause;
			break;

		case 'F':
			if(action == SGCT_PRESS)
				for(std::size_t i=0; i<gEngine->getNumberOfWindows(); i++)
				{
					gEngine->getWindowPtr(i)->setUseFXAA( gEngine->getWindowPtr(i)->useFXAA() );
				}
			break;

		case 'P':
		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot.setVal( true );
			break;

		case 'R':
			if(action == SGCT_PRESS)
				sgct_core::ClusterManager::Instance()->getThisNodePtr()->showAllWindows();
			break;
		}
	}
}