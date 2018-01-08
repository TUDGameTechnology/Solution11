#include <Kore/pch.h>

#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Math/Quaternion.h>
#include <Kore/Threads/Thread.h>
#include <Kore/Threads/Mutex.h>
#include <Kore/Network/Socket.h>
#include <Kore/Log.h>
#include "ObjLoader.h"

#include <sstream>

#include "MeshObject.h"

// uncomment to be in control of the game
//#define MASTER

#ifdef MASTER
#define SRC_PORT 9898
#define DEST_PORT 9897
#define CLIENT_NAME "Master"
#else
#define SRC_PORT 9897
#define DEST_PORT 9898
#define CLIENT_NAME "Slave"
#endif

#define DEST_IP1 127
#define DEST_IP2 0
#define DEST_IP3 0
#define DEST_IP4 1

using namespace Kore;

class Ball : public MeshObject {
public:
	Ball(float x, float y, float z, const Graphics4::VertexStructure& structure, float scale = 1.0f) : MeshObject("ball.obj", "unshaded.png", structure, scale), x(x), y(y), z(z), dir(0, 0, 0) {
		rotation = Quaternion(vec3(0, 0, 1), 0);
	}
	
	void update(float tdif) override {
		vec3 dir = this->dir;
		if (dir.getLength() != 0) dir.setLength(dir.getLength() * tdif * 60.0f);
			x += dir.x();
			if (x > 1) {
				x = 1;
			}
		if (x < -1) {
			x = -1;
		}
		y += dir.y();
		if (y < 0) {
			y = 4;
		}
		if (y > 4) {
			y = 0;
		}
		z += dir.z();
		if (dir.getLength() != 0) {
			float Horizontal = dir.dot(vec3(1, 0, 0));
			float Vertical = dir.dot(vec3(0, 1, 0));
			
			rotation = rotation.rotated(Quaternion(vec3(-1, 0, 0), Vertical * 3.0f));
			rotation = rotation.rotated(Quaternion(vec3(0, 1, 0), Horizontal * 3.0f));
		}
		M = mat4::Translation(x, y, z) * rotation.matrix();
	}
	
	vec3 dir;
	Quaternion rotation;
	float x, y, z;
};

namespace {
	void updateBall();
	
	const int width = 512;
	const int height = 512;
	
	double startTime;
	Graphics4::Shader* vertexShader;
	Graphics4::Shader* fragmentShader;
	Graphics4::PipelineState* pipeline;
	
	// null terminated array of MeshObject pointers
	MeshObject* objects[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	Ball* balls[] = { nullptr, nullptr, nullptr };
	
	// uniform locations - add more as you see fit
	Graphics4::TextureUnit tex;
	Graphics4::ConstantLocation pvLocation;
	Graphics4::ConstantLocation mLocation;
	
	mat4 PV;
	
	float lastTime = 0;
	
	Socket socket;
	vec3 position(0, 2, -3);
	
	const int port = SRC_PORT;
	const int destPort = DEST_PORT;
	const char* destination = "localhost";
	
	// Send a packet to the other client
	// If you are sending strings, make sure to null-terminate them, e.g. "hello\0"
	// length is the length of the packet in bytes
	void sendPacket(const unsigned char data[], int length) {
		socket.send(destination, destPort, data, length);
	}
	
	// Movement data for clients
	bool left = false, right = false, up = false, down = false;
	bool left2 = false, right2 = false, up2 = false, down2 = false;
	
	void update() {
		// receive packets
		while (true) {
			// Read buffer
			unsigned char buffer[256];
			float *floatBuffer = (float*) buffer;
			unsigned fromAddress;
			unsigned fromPort;
			int read = socket.receive(buffer, sizeof(buffer), fromAddress, fromPort);
			std::ostringstream ss;
			ss << buffer;
			
			// break if there was no new packet
			if (read <= 0) {
				break;
			}
			
			/************************************************************************/
			/* Practical Task: Read the packets with the movement data you sent  and
			 /* apply them by setting the boolean values for movement control.
			 /************************************************************************/
#ifdef MASTER
			// Set the values for left2, right2, up2, down2 here
			if (ss.str() == "left") {
				left2 = true;
			}
			if (ss.str() == "lStop") {
				left2 = false;
			}
			if (ss.str() == "right") {
				right2 = true;
			}
			if (ss.str() == "rStop") {
				right2 = false;
			}
			if (ss.str() == "up") {
				up2 = true;
			}
			if (ss.str() == "uStop") {
				up2 = false;
			}
			if (ss.str() == "down") {
				down2 = true;
			}
			if (ss.str() == "dStop") {
				down2 = false;
			}
#else
			// Set the values for left, right, up, down here
			if (ss.str() == "left") {
				left = true;
			}
			if (ss.str() == "lStop") {
				left = false;
			}
			if (ss.str() == "right") {
				right = true;
			}
			if (ss.str() == "rStop") {
				right = false;
			}
			if (ss.str() == "up") {
				up = true;
			}
			if (ss.str() == "uStop") {
				up = false;
			}
			if (ss.str() == "down") {
				down = true;
			}
			if (ss.str() == "dStop") {
				down = false;
			}
			
			// receive position updates of the npc ball
			if ((ss.str() == "x")) {
				read = socket.receive(buffer, sizeof(buffer), fromAddress, fromPort);
				balls[2]->x = floatBuffer[0];
			}
			if ((ss.str() == "y")) {
				read = socket.receive(buffer, sizeof(buffer), fromAddress, fromPort);
				balls[2]->y = floatBuffer[0];
			}
			if ((ss.str() == "z")) {
				read = socket.receive(buffer, sizeof(buffer), fromAddress, fromPort);
				balls[2]->z = floatBuffer[0];
			}
#endif // MASTER
			
			updateBall();
		}
		
		float t = (float)(System::time() - startTime);
		float tdif = t - lastTime;
		lastTime = t;
		
		updateBall();
		
		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, 0xff9999FF, 1.0f);
		
		Graphics4::setPipeline(pipeline);
		
		PV = mat4::Perspective(60, (float)width / (float)height, 0.1f, 100) * mat4::lookAt(position, vec3(position.x(), position.y(), position.z() + 10.0f), vec3(0, 1, 0));
		Graphics4::setMatrix(pvLocation, PV);
		
		MeshObject** current = &objects[0];
		while (*current != nullptr) {
			(*current)->update(tdif);
			Graphics4::setMatrix(mLocation, (*current)->M);
			(*current)->render(tex);
			++current;
		}
		
#ifdef MASTER
		// send position of the npc ball
		unsigned char floatData[255];
		float *f_buf = (float*)floatData;
		
		const unsigned char data1[] = "x\0";
		const unsigned char data2[] = "y\0";
		const unsigned char data3[] = "z\0";
		
		sendPacket(data1, sizeof(unsigned char) * 2);
		f_buf[0] = balls[2]->x;
		sendPacket(floatData, sizeof(float));
		sendPacket(data2, sizeof(unsigned char)* 2);
		f_buf[0] = balls[2]->y;
		sendPacket(floatData, sizeof(float));
		sendPacket(data3, sizeof(unsigned char)* 2);
		f_buf[0] = balls[2]->z;
		sendPacket(floatData, sizeof(float));
#endif
		
		Graphics4::end();
		Graphics4::swapBuffers();
	}
	
	void updateBall() {
		// user controlled balls
		float speed = 0.05f;
		if (left) {
			balls[0]->dir.x() = -speed;
		}
		else if (right) {
			balls[0]->dir.x() = speed;
		}
		else {
			balls[0]->dir.x() = 0;
		}
		if (up) {
			balls[0]->dir.y() = speed;
		}
		else if (down) {
			balls[0]->dir.y() = -speed;
		}
		else {
			balls[0]->dir.y() = 0;
		}
		if (left2) {
			balls[1]->dir.x() = -speed;
		}
		else if (right2) {
			balls[1]->dir.x() = speed;
		}
		else {
			balls[1]->dir.x() = 0;
		}
		if (up2) {
			balls[1]->dir.y() = speed;
		}
		else if (down2) {
			balls[1]->dir.y() = -speed;
		}
		else {
			balls[1]->dir.y() = 0;
		}
		// NPC ball
		balls[2]->dir.y() = -0.02f;
#ifdef MASTER
		if (balls[2]->y == 4) {
			balls[2]->x = ((float)rand() / RAND_MAX)*2-1;
		}
#endif
	}
	
	/************************************************************************/
	/* Practical Task: Send packets with information about the input controls
	 /* of the local player - keyDown
	 /************************************************************************/
	void keyDown(KeyCode code) {
#ifdef MASTER
		if (code == KeyLeft) {
			left = true;
			const unsigned char data[] = "left\0";
			sendPacket(data, sizeof(unsigned char) * 5);
		}
		else if (code == KeyRight) {
			right = true;
			const unsigned char data[] = "right\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyUp) {
			up = true;
			const unsigned char data[] = "up\0";
			sendPacket(data, sizeof(unsigned char)* 3);
		}
		else if (code == KeyDown) {
			down = true;
			const unsigned char data[] = "down\0";
			sendPacket(data, sizeof(unsigned char)* 5);
		}
#else
		if (code == KeyA) {
			left2 = true;
			const unsigned char data[] = "left\0";
			sendPacket(data, sizeof(unsigned char)* 5);
		}
		else if (code == KeyD) {
			right2 = true;
			const unsigned char data[] = "right\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		if (code == KeyW) {
			up2 = true;
			const unsigned char data[] = "up\0";
			sendPacket(data, sizeof(unsigned char)* 3);
		}
		else if (code == KeyS) {
			down2 = true;
			const unsigned char data[] = "down\0";
			sendPacket(data, sizeof(unsigned char)* 5);
		}
#endif // MASTER
	}
	
	/************************************************************************/
	/* Practical Task: Send packets with information about the input controls
	 /* of the local player - keyUp
	 /************************************************************************/
	void keyUp(KeyCode code) {
#ifdef MASTER
		if (code == KeyLeft) {
			left = false;
			const unsigned char data[] = "lStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyRight) {
			right = false;
			const unsigned char data[] = "rStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyUp) {
			up = false;
			const unsigned char data[] = "uStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyDown) {
			down = false;
			const unsigned char data[] = "dStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
#else
		if (code == KeyA) {
			left2 = false;
			const unsigned char data[] = "lStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyD) {
			right2 = false;
			const unsigned char data[] = "rStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyW) {
			up2 = false;
			const unsigned char data[] = "uStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
		else if (code == KeyS) {
			down2 = false;
			const unsigned char data[] = "dStop\0";
			sendPacket(data, sizeof(unsigned char)* 6);
		}
#endif // MASTER
	}
	
	void init() {
		srand(42);
		socket.init();
		socket.open(port);
		
		// send "hello" when joining to tell other player you are there
		const unsigned char data[] = "hello\0";
		sendPacket(data, sizeof(unsigned char)* 6);
		
#ifdef MASTER
		log(Info, "Waiting for another player (the SLAVE) to join my game...");
#else
		log(Info, "Waiting for another player (the MASTER) in control of the game...");
#endif // MASTER
		
		// wait for other player
		while (true) {
			// read buffer
			unsigned char buffer[256];
			unsigned fromAddress;
			unsigned fromPort;
			int read = socket.receive(buffer, sizeof(buffer), fromAddress, fromPort);
			std::ostringstream ss;
			ss << buffer;
			// break if player is there
			if (ss.str() == "hello\0") {
				break;
			}
		}
		
#ifdef MASTER
		log(Info, "Another player (the SLAVE) has joined my game!");
#else
		log(Info, "I have joined another players (the MASTER) game!");
#endif // MASTER
		
		// resend hello for newly connected player
		sendPacket(data, sizeof(unsigned char)* 6);
		
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Graphics4::Shader(vs.readAll(), vs.size(), Graphics4::VertexShader);
		fragmentShader = new Graphics4::Shader(fs.readAll(), fs.size(), Graphics4::FragmentShader);
		
		// This defines the structure of your Vertex Buffer
		Graphics4::VertexStructure structure;
		structure.add("pos", Graphics4::Float3VertexData);
		structure.add("tex", Graphics4::Float2VertexData);
		structure.add("nor", Graphics4::Float3VertexData);
		
		pipeline = new Graphics4::PipelineState;
		pipeline->inputLayout[0] = &structure;
		pipeline->inputLayout[1] = nullptr;
		pipeline->vertexShader = vertexShader;
		pipeline->fragmentShader = fragmentShader;
		pipeline->depthMode = Graphics4::ZCompareLess;
		pipeline->depthWrite = true;
		pipeline->compile();
		
		tex = pipeline->getTextureUnit("tex");
		pvLocation = pipeline->getConstantLocation("PV");
		mLocation = pipeline->getConstantLocation("M");
		
		objects[0] = balls[0] = new Ball(0.5f, 1.0f, 0.0f, structure, 3.0f);
		objects[1] = balls[1] = new Ball(-0.5f, 1.0f, 0.0f, structure, 3.0f);
		
		objects[2] = balls[2] = new Ball(((float)rand() / RAND_MAX)*2-1, 4.0f, 0.0f, structure, 3.0f);
		objects[3] = new MeshObject("base.obj", "floor.png", structure);
		objects[3]->M = mat4::RotationX(3.1415f / 2.0f)*mat4::Scale(0.15f, 1, 1);
		objects[4] = new MeshObject("base.obj", "StarMap.png", structure);
		objects[4]->M = mat4::RotationX(3.1415f / 2.0f)*mat4::Scale(1, 1, 1)*mat4::Translation(0, 0, 0.5f);
		
		Graphics4::setTextureAddressing(tex, Graphics4::U, Graphics4::Repeat);
		Graphics4::setTextureAddressing(tex, Graphics4::V, Graphics4::Repeat);
	}
}

int kore(int argc, char** argv) {
#ifdef MASTER
	log(Info, "I am the MASTER, I am in control of the game.");
#else
	log(Info, "I am the SLAVE, I want to join another game.");
#endif // MASTER
	
	log(Info, "I am listening on port %i", port);
	log(Info, "and want to connect to %s:%i\n", destination, destPort);
	
	Kore::System::init("Exercise 11 - "  CLIENT_NAME, width, height);
	
	init();
	
	Kore::System::setCallback(update);
	
	startTime = System::time();
	
	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	
	Kore::System::start();
	
	return 0;
}
