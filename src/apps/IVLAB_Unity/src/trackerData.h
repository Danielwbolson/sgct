#pragma once
// Definitions for tracker data

#include "json11.hpp"

class Vector3 {
public:
	float x;
	float y;
	float z;
	json11::Json to_json() const {
		return json11::Json::object{
			{"x", x},
			{"y", y},
			{"z", z},
		};
	}
};

class Quaternion {
public:
	float x;
	float y;
	float z;
	float w;

	json11::Json to_json() const {
		return json11::Json::object{
			{"x", x},
			{"y", y},
			{"z", z},
			{"w", w},
		};
	}
};

class TrackerData {
public:
	Vector3 position;
	Quaternion orientation;
	double vrpnCallbackTime;
	double dataSendTime;

	// Buf must be allocated somewhere else with at least 28 bytes.
	void posRotBytes(char* buf) {
		memset(buf, 0x0, sizeof(Vector3) + sizeof(Quaternion));
		char* posBytes = reinterpret_cast<char*>(&position);
		char* rotBytes = reinterpret_cast<char*>(&orientation);
		memcpy(buf, posBytes, sizeof(Vector3));
		memcpy(buf + sizeof(Vector3), rotBytes, sizeof(Quaternion));
	}

	// Position, rotation, plus timing information (at least 44 bytes)
	void posRotTimingBytes(char* buf) {
		memset(buf, 0x0, sizeof(Vector3) + sizeof(Quaternion) + 2*sizeof(double));
		char* posBytes = reinterpret_cast<char*>(&position);
		char* rotBytes = reinterpret_cast<char*>(&orientation);
		char* vrpnBytes = reinterpret_cast<char*>(&vrpnCallbackTime);
		char* sendBytes = reinterpret_cast<char*>(&dataSendTime);
		memcpy(buf, posBytes, sizeof(Vector3));
		memcpy(buf + sizeof(Vector3), rotBytes, sizeof(Quaternion));
		memcpy(buf + sizeof(Vector3) + sizeof(Quaternion), vrpnBytes, sizeof(double));
		memcpy(buf + sizeof(Vector3) + sizeof(Quaternion) + sizeof(double), sendBytes, sizeof(double));
	}

	json11::Json to_json() const {
		return json11::Json::object {
			{"position", position.to_json()},
			{"orientation", orientation.to_json()},
			{"vrpnCallbackTime", (double) vrpnCallbackTime},
			{"dataSendTime", (double) dataSendTime},
		};
	}
};