/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Link�ping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#if !(_MSC_VER >= 1400) //if not visual studio 2005 or later
    #define _WIN32_WINNT 0x501
#endif

#ifdef __WIN32__
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else //Use BSD sockets
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#define SOCKET_ERROR (-1)
	#define INVALID_SOCKET (SOCKET)(~0)
	#define NO_ERROR 0L
#endif

#include <GL/glfw.h>
#include "../include/sgct/SGCTNetwork.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/Engine.h"
#include <stdlib.h>
#include <stdio.h>

void GLFWCALL communicationHandler(void *arg);
void GLFWCALL connectionHandler(void *arg);

#define MAX_NUMBER_OF_ATTEMPS 10

core_sgct::SGCTNetwork::SGCTNetwork()
{
	mCommThreadId		= -1;
	mMainThreadId		= -1;
	mSocket				= INVALID_SOCKET;
	mListenSocket		= INVALID_SOCKET;
	mDecoderCallbackFn	= NULL;
	mUpdateCallbackFn	= NULL;
	mConnectedCallbackFn = NULL;
	mServerType			= SyncServer;
	mBufferSize			= 1024;
	mRequestedSize		= mBufferSize;
	mSendFrame			= 0;
	mRecvFrame[0]		= 0;
	mRecvFrame[1]		= 0;
	mConnected			= false;
	mTerminate          = false;
	mId					= -1;
	mConnectionMutex	= NULL;
	mDoneCond			= NULL;
}

void core_sgct::SGCTNetwork::init(const std::string port, const std::string ip, bool _isServer, int id, int serverType)
{
	mServer = _isServer;
	mServerType = serverType;
	mBufferSize = sgct::SharedData::Instance()->getBufferSize();
	mId = id;

	mConnectionMutex = sgct::Engine::createMutex();
	mDoneCond = sgct::Engine::createCondition();
    if(mConnectionMutex == NULL || mDoneCond == NULL)
        throw "Failed to create connection mutex & condition.";

	struct addrinfo *result = NULL, *ptr = NULL, hints;
#ifdef __WIN32__ //WinSock
	ZeroMemory(&hints, sizeof (hints));
#else
	memset(&hints, 0, sizeof (hints));
#endif
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult;

	if( mServer )
		iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
	else
		iResult = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0)
	{
		//WSACleanup(); hanteras i manager
		throw "Failed to parse hints for connection.";
	}

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	if( mServer )
	{
		// Create a SOCKET for the server to listen for client connections
		mListenSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (mListenSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			throw "Failed to listen init socket!";
		}

		setOptions( &mListenSocket );

		// Setup the TCP listening socket
		iResult = bind( mListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			freeaddrinfo(result);
#ifdef __WIN32__
			closesocket(mListenSocket);
#else
			close(mListenSocket);
#endif
			throw "Bind socket failed!";
		}

		if( listen( mListenSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
            freeaddrinfo(result);
#ifdef __WIN32__
			closesocket(mListenSocket);
#else
			close(mListenSocket);
#endif
			throw "Listen failed!";
		}
	}
	else
	{
		//Client socket

		// Connect to server.
		while( true )
		{
			sgct::MessageHandler::Instance()->print("Attempting to connect to server...\n");

			mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (mSocket == INVALID_SOCKET)
            {
                freeaddrinfo(result);
                throw "Failed to init client socket!";
            }

            setOptions( &mSocket );

			iResult = connect( mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR)
				break;

			glfwSleep(1.0); //wait for next attempt
		}
	}

	freeaddrinfo(result);
	mMainThreadId = glfwCreateThread( connectionHandler, this );
	if( mMainThreadId < 0)
	{
#ifdef __WIN32__
		mServer ? closesocket(mListenSocket) : closesocket(mSocket);
#else
		mServer ? close(mListenSocket) : close(mSocket);
#endif
		throw "Failed to start main network thread!";
	}
	/*else
        sgct::MessageHandler::Instance()->print("Main thread created, id=%d\n", mMainThreadId);*/
}

void GLFWCALL connectionHandler(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	if( nPtr->isServer() )
	{
		while( !nPtr->isTerminated() )
		{
			if( !nPtr->isConnected() )
			{
                //first time the thread is -1 so the wait will not run
				if( nPtr->mCommThreadId != -1 )
				{
				    //wait for the connection to disconnect
				    glfwWaitThread( nPtr->mCommThreadId, GLFW_WAIT );

                    //reset thread handle/id
				    nPtr->mCommThreadId = -1;
				}

                //start a new connection enabling the client to reconnect
				nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );
				if( nPtr->mCommThreadId < 0)
				{
					return;
				}
				/*else
                    sgct::MessageHandler::Instance()->print("Comm thread created, id=%d\n", nPtr->mCommThreadId);*/
			}

            //wait for signal until next iteration in loop
            if( !nPtr->isTerminated() )
            {
                sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                    sgct::Engine::waitCond( core_sgct::NetworkManager::gStartConnectionCond,
                        nPtr->mConnectionMutex,
                        GLFW_INFINITY );
                sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
            }
		}
	}
	else //if client
	{
		nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );

		if( nPtr->mCommThreadId < 0)
		{
			return;
		}
		/*else
            sgct::MessageHandler::Instance()->print("Comm thread created, id=%d\n", nPtr->mCommThreadId);*/
	}

	sgct::MessageHandler::Instance()->print("Closing connection handler for connection %d... \n", nPtr->getId());
}

void core_sgct::SGCTNetwork::setOptions(SOCKET * socketPtr)
{
	if(socketPtr != NULL)
	{
		int flag = 1;

		//set no delay, disable nagle's algorithm
		int iResult = setsockopt(*socketPtr, /* socket affected */
		IPPROTO_TCP,     /* set option at TCP level */
		TCP_NODELAY,     /* name of option */
		(char *) &flag,  /* the cast is historical cruft */
		sizeof(int));    /* length of option value */

        if( iResult != NO_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to set no delay with error: %d\nThis will reduce cluster performance!", errno);

		//set timeout
		int timeout = 0; //infinite
        iResult = setsockopt(
                *socketPtr,
                SOL_SOCKET,
                SO_SNDTIMEO,
                (char *)&timeout,
                sizeof(timeout));

		//iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_DONTLINGER, (char*)&flag, sizeof(int));
        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));
	}
}

void core_sgct::SGCTNetwork::closeSocket(SOCKET lSocket)
{
    if( lSocket != INVALID_SOCKET )
	{
	    /*
		Windows shutdown options
            * SD_RECIEVE
            * SD_SEND
            * SD_BOTH

        Linux & Mac shutdown options
            * SHUT_RD (Disables further receive operations)
            * SHUT_WR (Disables further send operations)
            * SHUT_RDWR (Disables further send and receive operations)
		*/

        sgct::Engine::lockMutex(mConnectionMutex);
#ifdef __WIN32__
        shutdown(lSocket, SD_BOTH);
		closesocket( lSocket );
#else
		shutdown(lSocket, SHUT_RDWR);
		close( lSocket );
#endif

		lSocket = INVALID_SOCKET;
		sgct::Engine::unlockMutex(mConnectionMutex);
	}
}

void core_sgct::SGCTNetwork::setBufferSize(unsigned int newSize)
{
	mRequestedSize = newSize;
}

void core_sgct::SGCTNetwork::iterateFrameCounter()
{
	if( mSendFrame < MAX_NET_SYNC_FRAME_NUMBER )
		mSendFrame++;
	else
		mSendFrame = 0;
}

void core_sgct::SGCTNetwork::pushClientMessage()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::pushClientMessage\n");
#endif

	//The servers's render function is locked until a message starting with the ack-byte is received.
	int currentFrame = getSendFrame();
	unsigned char *p = (unsigned char *)&currentFrame;

    if(sgct::MessageHandler::Instance()->getDataSize() > mHeaderSize)
    {
        //Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::Instance()->getMessage();
		messageToSend[0] = SGCTNetwork::SyncByte;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];

		unsigned int currentMessageSize =
			sgct::MessageHandler::Instance()->getDataSize() > mBufferSize ?
			mBufferSize :
			sgct::MessageHandler::Instance()->getDataSize();

        unsigned int dataSize = currentMessageSize - mHeaderSize;
		unsigned char *currentMessageSizePtr = (unsigned char *)&dataSize;
		messageToSend[5] = currentMessageSizePtr[0];
		messageToSend[6] = currentMessageSizePtr[1];
		messageToSend[7] = currentMessageSizePtr[2];
		messageToSend[8] = currentMessageSizePtr[3];

		//crop if needed
		if( sendData((void*)messageToSend, currentMessageSize) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send sync header + client log message!\n");

		sgct::MessageHandler::Instance()->clearBuffer(); //clear the buffer
    }
	else
	{
		char tmpca[mHeaderSize];
		tmpca[0] = SGCTNetwork::SyncByte;
		tmpca[1] = p[0];
		tmpca[2] = p[1];
		tmpca[3] = p[2];
		tmpca[4] = p[3];

		unsigned int localSyncHeaderSize = 0;
		unsigned char *currentMessageSizePtr = (unsigned char *)&localSyncHeaderSize;
		tmpca[5] = currentMessageSizePtr[0];
		tmpca[6] = currentMessageSizePtr[1];
		tmpca[7] = currentMessageSizePtr[2];
		tmpca[8] = currentMessageSizePtr[3];

		if( sendData((void *)tmpca, mHeaderSize) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send sync header!\n");
	}
}

int core_sgct::SGCTNetwork::getSendFrame()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getSendFrame\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mSendFrame;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

bool core_sgct::SGCTNetwork::compareFrames()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::compareFrames\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = (mRecvFrame[0] == mRecvFrame[1]);
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

void core_sgct::SGCTNetwork::swapFrames()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::swapFrames\n");
#endif
	sgct::Engine::lockMutex(mConnectionMutex);
		mRecvFrame[1] = mRecvFrame[0];
	sgct::Engine::unlockMutex(mConnectionMutex);
}

void core_sgct::SGCTNetwork::setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setUpdateFunction(std::tr1::function<void (int)> callback)
{
	mUpdateCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setConnectedFunction(std::tr1::function<void (void)> callback)
{
	mConnectedCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setConnectedStatus(bool state)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::setConnectedStatus = %s at syncframe %d\n", state ? "true" : "false", getSendFrame());
#endif
	sgct::Engine::lockMutex(mConnectionMutex);
		mConnected = state;
	sgct::Engine::unlockMutex(mConnectionMutex);
}

bool core_sgct::SGCTNetwork::isConnected()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::isConnected\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mConnected;
	sgct::Engine::unlockMutex(mConnectionMutex);
    return tmpb;
}

int core_sgct::SGCTNetwork::getTypeOfServer()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getTypeOfServer\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mServerType;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

int core_sgct::SGCTNetwork::getId()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getId\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mId;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

bool core_sgct::SGCTNetwork::isServer()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::isServer\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mServer;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

bool core_sgct::SGCTNetwork::isTerminated()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::isTerminated\n");
#endif
    bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mTerminate;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

void core_sgct::SGCTNetwork::setRecvFrame(int i)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::setRecvFrame\n");
#endif
	sgct::Engine::lockMutex(mConnectionMutex);
	mRecvFrame[0] = i;
	sgct::Engine::unlockMutex(mConnectionMutex);
}

int core_sgct::SGCTNetwork::receiveData(SOCKET & lsocket, char * buffer, int length, int flags)
{
    int iResult = 0;
    static int attempts = 1;

    while( iResult < length )
    {
        int tmpRes = recv( lsocket, buffer + iResult, length - iResult, flags);
#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::Instance()->print("Received %d bytes of %d...\n", tmpRes, length);
        for(int i=0; i<tmpRes; i++)
            sgct::MessageHandler::Instance()->print("%u\t", buffer[i]);
        sgct::MessageHandler::Instance()->print("\n");
#endif

        if( tmpRes > 0 )
            iResult += tmpRes;
#ifdef __WIN32__
        else if( errno == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#else
        else if( errno == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#endif
        {
            sgct::MessageHandler::Instance()->print("Receiving data after interrupted system error (attempt %d)...\n", attempts);
            //iResult = 0;
            attempts++;
        }
        else
        {
            //capture error
            iResult = tmpRes;
            break;
        }
    }

    return iResult;
}

int core_sgct::SGCTNetwork::parseInt(char * str)
{
    union
    {
        int i;
        char c[4];
    } ci;

    ci.c[0] = str[0];
    ci.c[1] = str[1];
    ci.c[2] = str[2];
    ci.c[3] = str[3];

    return ci.i;
}

unsigned int core_sgct::SGCTNetwork::parseUnsignedInt(char * str)
{
    union
    {
        unsigned int ui;
        char c[4];
    } cui;

    cui.c[0] = str[0];
    cui.c[1] = str[1];
    cui.c[2] = str[2];
    cui.c[3] = str[3];

    return cui.ui;
}

/*
function to decode messages
*/
void GLFWCALL communicationHandler(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	//exit if terminating
	if( nPtr->isTerminated() )
        return;

	//listen for client if server
	if( nPtr->isServer() )
	{
		sgct::MessageHandler::Instance()->print("Waiting for client to connect to connection %d...\n", nPtr->getId());

        nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);

#ifdef __WIN32__
        int accErr = WSAGetLastError();
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        int accErr = errno;
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == EINTR)
#endif
		{
		    sgct::MessageHandler::Instance()->print("Re-accept after interrupted system on connection %d...\n", nPtr->getId());

		    nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		}

		if (nPtr->mSocket == INVALID_SOCKET)
		{
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Accept connection %d failed! Error: %d\n", nPtr->getId(), accErr);
#endif
			if(nPtr->mUpdateCallbackFn != NULL)
                nPtr->mUpdateCallbackFn( nPtr->getId() );
			return;
		}
	}

	nPtr->setConnectedStatus(true);
	sgct::MessageHandler::Instance()->print("Connection %d established!\n", nPtr->getId());

	if(nPtr->mUpdateCallbackFn != NULL)
		nPtr->mUpdateCallbackFn( nPtr->getId() );

	//init buffers
	char recvHeader[core_sgct::SGCTNetwork::mHeaderSize];
	memset(recvHeader, core_sgct::SGCTNetwork::FillByte, core_sgct::SGCTNetwork::mHeaderSize);
	char * recvBuf = NULL;

	//recvbuf = reinterpret_cast<char *>( malloc(nPtr->mBufferSize) );
	recvBuf = new (std::nothrow) char[nPtr->mBufferSize];
	std::string extBuffer; //for external comm

	// Receive data until the server closes the connection
	int iResult = 0;
	do
	{
		//resize buffer request
		if( nPtr->mRequestedSize > nPtr->mBufferSize )
		{
			#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Re-sizing tcp buffer size from %d to %d... ", nPtr->mBufferSize, nPtr->mRequestedSize);
            #endif

            sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", nPtr->mRequestedSize);
			sgct::Engine::lockMutex(nPtr->mConnectionMutex);
				nPtr->mBufferSize = nPtr->mRequestedSize;

				//clean up
                delete [] recvBuf;
                recvBuf = NULL;

                //allocate
                bool allocError = false;
                recvBuf = new (std::nothrow) char[nPtr->mRequestedSize];
                if(recvBuf == NULL)
                    allocError = true;

			sgct::Engine::unlockMutex(nPtr->mConnectionMutex);

			if(allocError)
				sgct::MessageHandler::Instance()->print("Network error: Buffer failed to resize!\n");
			else
				sgct::MessageHandler::Instance()->print("Network: Buffer resized successfully!\n");

			#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Done.\n");
            #endif
		}

#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("Receiving message header...\n");
#endif

        /*
            Get & parse the message header if not external control
        */
        int syncFrameNumber = -1;
        unsigned int dataSize = 0;
        unsigned char packageId = core_sgct::SGCTNetwork::FillByte;

        if( nPtr->getTypeOfServer() != core_sgct::SGCTNetwork::ExternalControl )
        {
            iResult = core_sgct::SGCTNetwork::receiveData(nPtr->mSocket,
                               recvHeader,
                               static_cast<int>(core_sgct::SGCTNetwork::mHeaderSize),
                               0);
        }

#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::Instance()->print("Header: %u | %u %u %u %u | %u %u %u %u\n",
                                                recvHeader[0],
                                                recvHeader[1],
                                                recvHeader[2],
                                                recvHeader[3],
                                                recvHeader[4],
                                                recvHeader[5],
                                                recvHeader[6],
                                                recvHeader[7],
                                                recvHeader[8]);
#endif

        if( iResult == static_cast<int>(core_sgct::SGCTNetwork::mHeaderSize))
        {
            packageId = recvHeader[0];
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Package id=%d...\n", packageId);
#endif
            if( packageId == core_sgct::SGCTNetwork::SyncByte )
            {
                //parse the sync frame number
                syncFrameNumber = core_sgct::SGCTNetwork::parseInt(&recvHeader[1]);
                //parse the data size
                dataSize = core_sgct::SGCTNetwork::parseUnsignedInt(&recvHeader[5]);

                //resize buffer if needed
                sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                if( dataSize > nPtr->mBufferSize )
                {
                    //clean up
                    delete [] recvBuf;
                    recvBuf = NULL;

                    //allocate
                    recvBuf = new (std::nothrow) char[dataSize];
                    if(recvBuf != NULL)
                    {
                        nPtr->mBufferSize = dataSize;
                    }
                }

                sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
            }
        }


#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("Receiving data (buffer size: %d)...\n", dataSize);
#endif
        /*
            Get the data/message
        */
        if( dataSize > 0 )
        {
			iResult = core_sgct::SGCTNetwork::receiveData(nPtr->mSocket,
                                        recvBuf,
                                        dataSize,
                                        0);
#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("Data type:% %d bytes of %u...\n", packageId, iResult, dataSize);
#endif
        }

		if( nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl )
		{
			iResult = recv( nPtr->mSocket,
                                        recvBuf,
                                        nPtr->mBufferSize,
                                        0);
		}

		if (iResult > 0)
		{
            //game over message
			if( packageId == core_sgct::SGCTNetwork::DisconnectByte &&
                recvHeader[1] == 24 &&
                recvHeader[2] == '\r' &&
                recvHeader[3] == '\n' &&
                recvHeader[4] == 27 &&
                recvHeader[5] == '\r' &&
                recvHeader[6] == '\n' &&
                recvHeader[7] == '\0' )
            {
                nPtr->setConnectedStatus(false);

                /*
                    Terminate client only. The server only resets the connection,
                    allowing clients to connect.
                */
                if( !nPtr->isServer() )
                {
                    sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                        nPtr->mTerminate = true;
                    sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
                }

                sgct::MessageHandler::Instance()->print("Disconnecting (from other peer)...\n");

                break; //exit loop
            }
			else if( nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::SyncServer )
			{
				if( packageId == core_sgct::SGCTNetwork::SyncByte &&
					nPtr->mDecoderCallbackFn != NULL)
				{
					nPtr->setRecvFrame( syncFrameNumber );
					if( syncFrameNumber < 0 )
					{
						sgct::MessageHandler::Instance()->print("Network: Error sync in sync frame: %d for connection %d\n", syncFrameNumber, nPtr->getId());
					}

#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Package info: Frame = %d, Size = %u\n", syncFrameNumber, dataSize);
#endif

                    //decode callback
                    if(dataSize > 0)
                        (nPtr->mDecoderCallbackFn)(recvBuf, dataSize, nPtr->getId());

					sgct::Engine::signalCond( core_sgct::NetworkManager::gCond );
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Done.\n");
#endif
				}
				else if( packageId == core_sgct::SGCTNetwork::ConnectedByte &&
					nPtr->mConnectedCallbackFn != NULL)
				{
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Signaling slave is connected... ");
#endif
					(nPtr->mConnectedCallbackFn)();
					sgct::Engine::signalCond( core_sgct::NetworkManager::gCond );
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Done.\n");
#endif
				}
			}
			else if(nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl)
			{
#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Parsing external tcp data... ");
#endif
				std::string tmpStr(recvBuf);
				extBuffer += tmpStr.substr(0, iResult);
				std::size_t found = extBuffer.find("\r\n");
				while( found != std::string::npos )
				{
					std::string extMessage = extBuffer.substr(0,found);
					extBuffer = extBuffer.substr(found+2);//jump over \r\n

					sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
					if( nPtr->mDecoderCallbackFn != NULL )
					{
						(nPtr->mDecoderCallbackFn)(extMessage.c_str(), extMessage.size(), nPtr->getId());
					}
					nPtr->sendStr("OK\r\n");
					sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
                    found = extBuffer.find("\r\n");
				}
#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Done.\n");
#endif
			}
		}
		else if (iResult == 0)
		{
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Done.\n");
#endif

#ifdef __WIN32__
			sgct::MessageHandler::Instance()->print("TCP Connection %d closed (error: %d)\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("TCP Connection %d closed (error: %d)\n", nPtr->getId(), errno);
#endif
		}
		else
		{
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Done.\n");
#endif

#ifdef __WIN32__
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), errno);
#endif
		}

	} while (iResult > 0 || nPtr->isConnected());


	//cleanup
	sgct::Engine::lockMutex(nPtr->mConnectionMutex);
	if( recvBuf != NULL )
    {
        //free(recvbuf);
        delete [] recvBuf;
        recvBuf = NULL;
    }
    sgct::Engine::unlockMutex(nPtr->mConnectionMutex);

	//Close socket
	nPtr->closeSocket( nPtr->mSocket );
	if(nPtr->mUpdateCallbackFn != NULL)
		nPtr->mUpdateCallbackFn( nPtr->getId() );

	if( nPtr->isServer() )
	{
		sgct::Engine::signalCond( core_sgct::NetworkManager::gStartConnectionCond );
	}

	sgct::MessageHandler::Instance()->print("Node %d disconnected!\n", nPtr->getId());
}

int core_sgct::SGCTNetwork::sendData(void * data, int length)
{
	//fprintf(stderr, "Send data size: %d\n", length);
#ifdef __SGCT_NETWORK_DEBUG__
	for(int i=0; i<length; i++)
        fprintf(stderr, "%u ", ((const char *)data)[i]);
    fprintf(stderr, "\n");
#endif
	return send(mSocket, (const char *)data, length, 0);
}

int core_sgct::SGCTNetwork::sendStr(std::string msg)
{
	//fprintf(stderr, "Send message: %s, size: %d\n", msg.c_str(), msg.size());
	return send(mSocket, msg.c_str(), msg.size(), 0);
}

void core_sgct::SGCTNetwork::closeNetwork(bool forced)
{
    if( mCommThreadId != -1 )
    {
        if( forced )
            glfwDestroyThread(mCommThreadId); //blocking sockets -> cannot wait for thread so just kill it brutally
        else
            glfwWaitThread(mCommThreadId, GLFW_WAIT);
    }

    if( mMainThreadId != -1 )
	{
        if( forced )
            glfwDestroyThread( mMainThreadId );
        else
            glfwWaitThread(mMainThreadId, GLFW_WAIT);
    }

	//wait for everything to really close!
	//Otherwise the mutex can be called after
	//it is destroyed.
	sgct::Engine::lockMutex(mConnectionMutex);
    sgct::Engine::waitCond( mDoneCond,
        mConnectionMutex,
        0.25 );
    sgct::Engine::unlockMutex(mConnectionMutex);

    if( mConnectionMutex != NULL )
	{
		sgct::Engine::destroyMutex(mConnectionMutex);
		mConnectionMutex = NULL;
	}

	if( mDoneCond != NULL )
	{
		sgct::Engine::destroyCond(mDoneCond);
		mDoneCond = NULL;
	}

	sgct::MessageHandler::Instance()->print("Connection %d successfully terminated.\n", mId);
}

void core_sgct::SGCTNetwork::initShutdown()
{
	if( isConnected() )
	{
        char gameOver[7];
        gameOver[0] = 24; //ASCII for cancel
        gameOver[1] = '\r';
        gameOver[2] = '\n';
        gameOver[3] = 27; //ASCII for Esc
        gameOver[4] = '\r';
        gameOver[5] = '\n';
        gameOver[6] = '\0';
        sendData(gameOver, 7);
	}

	sgct::MessageHandler::Instance()->print("Closing connection %d... \n", getId());
	sgct::Engine::lockMutex(mConnectionMutex);
        mTerminate = true;
        mDecoderCallbackFn = NULL;
        mConnected = false;
	sgct::Engine::unlockMutex(mConnectionMutex);

	//wake up the connection handler thread (in order to finish)
	if( isServer() )
	{
		sgct::Engine::signalCond( core_sgct::NetworkManager::gStartConnectionCond );
	}

    closeSocket( mSocket );
    closeSocket( mListenSocket );
}
