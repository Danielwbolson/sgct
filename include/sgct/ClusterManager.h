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

#ifndef _CLUSTER_MANAGER
#define _CLUSTER_MANAGER

#include "SGCTNode.h"
#include "User.h"
#include "SGCTTracking.h"
#include <vector>
#include <string>

namespace core_sgct
{

class ClusterManager
{
public:
	/*! Get the ClusterManager instance */
	static ClusterManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new ClusterManager();
		}

		return mInstance;
	}

	/*! Destroy the ClusterManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void addNode(SGCTNode node);
	unsigned int getNumberOfNodes() const { return nodes.size(); }
	SGCTNode * getNodePtr(unsigned int index);
	SGCTNode * getThisNodePtr();
	User * getUserPtr() { return mUser; }
	SGCTTracking * getTrackingPtr() { return mTracking; }
	void setThisNodeId(int id) { mThisNodeId = id; }
	int getThisNodeId() { return mThisNodeId; }

	std::string * getMasterIp() { return &masterIp; }
	void setMasterIp(std::string & ip) { masterIp.assign(ip); }

	std::string * getExternalControlPort() { return &mExternalControlPort; }
	void setExternalControlPort(std::string & port) { mExternalControlPort.assign(port); }

private:
	ClusterManager(void);
	~ClusterManager(void);

	// Don't implement these, should give compile warning if used
	ClusterManager( const ClusterManager & nm );
	const ClusterManager & operator=(const ClusterManager & nm );

private:
	static ClusterManager * mInstance;

	std::vector<SGCTNode> nodes;
	int masterIndex;
	int mThisNodeId;
	bool validCluster;
	std::string masterIp;
	std::string mExternalControlPort;

	User	* mUser;
	SGCTTracking * mTracking;
};
}

#endif
