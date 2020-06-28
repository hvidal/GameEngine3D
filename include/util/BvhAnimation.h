#ifndef INCLUDE_UTIL_BVHANIMATION_H_
#define INCLUDE_UTIL_BVHANIMATION_H_

#include <string>
#include <vector>
#include "../scene/SceneObject.h"
#include "IoUtils.h"
#include "StringUtils.h"
#include "math/Matrix4x4.h"

// DOC: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.103.2097&rep=rep1&type=pdf
// DOWNLOADS: https://sites.google.com/a/cgspeed.com/cgspeed/motion-capture/daz-friendly-release

class BvhAnimation
{
	struct Node {
		std::string name;
		btVector3 offset;
		int channels;
		Node* parent;
		std::vector<Node*> children;
		std::vector<btVector3> framePosition;
		std::vector<btVector3> frameRotation;
		Matrix4x4 matrix;
	};

	Node* mRoot;
	std::vector<Node*> mNodes;
	unsigned int mFrameCount;
	float mFrameTime;
	int mCurrentFrame;
	btVector3 mMovementFactor;

	Uint32 mStartTime;

	virtual void _assert(const std::string& value1, const char* value2) {
		if (value1.compare(value2) != 0)
			Log::error("Error, %s not found", value2);
	}

	virtual Node* parseNode(std::vector<std::string> lines, int &index, Node* parent) {
		if (lines[index].compare("MOTION") == 0 || lines[index].compare("}") == 0) {
			index++;
			return nullptr;
		}
		char name[100];
		float x, y, z;
		int channels = 0;
		if (lines[index].find("End Site") == 0) {
			strcpy(name, lines[index++].c_str());
			_assert(lines[index++], "{");
			sscanf(lines[index++].c_str(), "OFFSET %g %g %g", &x, &y, &z);
		} else {
			sscanf(lines[index++].c_str(), parent == nullptr? "ROOT %s" : "JOINT %s", name);
			_assert(lines[index++], "{");
			sscanf(lines[index++].c_str(), "OFFSET %g %g %g", &x, &y, &z);
			sscanf(lines[index++].c_str(), "CHANNELS %d", &channels);
		}

		Log::debug("Node %s", name);

		Node* node = new Node();
		node->name = std::string(name);
		node->offset = btVector3(x, y, z);
		node->channels = channels;
		node->parent = parent;
		mNodes.push_back(node);

		Node* child = parseNode(lines, index, node);
		while (child != nullptr) {
			node->children.push_back(child);
			child = parseNode(lines, index, node);
		}
		return node;
	}

	virtual void parseFrame(int pos, Node* node, std::vector<std::string> values) {
		if (node->name.compare("End Site") == 0)
			return;
		if (node->channels == 3) {
			float rz = ::atof(values[pos++].c_str());
			float rx = ::atof(values[pos++].c_str());
			float ry = ::atof(values[pos++].c_str());
			node->frameRotation.push_back(btVector3(rx, ry, rz));
		} else if (node->channels == 6) {
			float px = ::atof(values[pos++].c_str());
			float py = ::atof(values[pos++].c_str());
			float pz = ::atof(values[pos++].c_str());
			node->framePosition.push_back(btVector3(px, py, pz));
			float rz = ::atof(values[pos++].c_str());
			float rx = ::atof(values[pos++].c_str());
			float ry = ::atof(values[pos++].c_str());
			node->frameRotation.push_back(btVector3(rx, ry, rz));
		}
	}

	virtual void parse(std::string contents) {
		std::vector<std::string> lines = StringUtils::split(contents);

		int index = 0;
		_assert(lines[index++], "HIERARCHY");

		mRoot = parseNode(lines, index, nullptr);

		Log::debug("motion index = %d", index);

		_assert(lines[index++], "MOTION");
		sscanf(lines[index++].c_str(), "Frames: %d", &mFrameCount);
		sscanf(lines[index++].c_str(), "Frame Time: %g", &mFrameTime);
		mFrameTime *= 1000;

		Log::debug("Frames = %d, Time = %.5f ms, Lines = %d", mFrameCount, mFrameTime, lines.size());

		while (index < lines.size()) {
			std::string line = lines[index++];
			if (line[0] == '#')
				continue;
			//Log::debug("Motion line %d: %s", index-1, line.c_str());
			std::vector<std::string> values = StringUtils::split(line, " ");
			int pos = 0;
			for (Node* node : mNodes) {
				parseFrame(pos, node, values);
				pos += node->channels;
			}
		}
		Log::debug("BVH Parse finished");
	}

	virtual void updateFrame(Node* node) {
		node->matrix.identity();

		btVector3 offset = btVector3(node->offset);
		if (node->framePosition.size() > 0) {
			offset += mMovementFactor * node->framePosition[mCurrentFrame];
		}
		node->matrix.translate(offset);

		if (node->frameRotation.size() > 0) {
			btVector3 rot = node->frameRotation[mCurrentFrame];

			rot = rot * 0.0174532925199; // convert to radians = PI / 180
			node->matrix.rotate(rot.z(), 0.f, 0.f, 1.f);
			node->matrix.rotate(rot.x(), 1.f, 0.f, 0.f);
			node->matrix.rotate(rot.y(), 0.f, 1.f, 0.f);
		}

		if (node->parent) {
			node->matrix.multiplyLeft(node->parent->matrix);
		}

		for (Node* child : node->children) {
			updateFrame(child);
		}
	}
public:
	BvhAnimation(const std::string &bvhFile):
		mCurrentFrame(0),
		mMovementFactor(btVector3(0.0, 1.0, 0.0)),
		mStartTime(0)
	{
		auto contents(IoUtils::readFile(bvhFile));
		parse(contents);
	}
	virtual ~BvhAnimation() {
		for (Node* node : mNodes)
			delete node;
	}

	virtual void ready() {
		updateFrame(mRoot);
	}

	virtual void normalize(float scale) {
		// scale first
		const btVector3 fix(scale, scale, scale);
		for (Node* node : mNodes) {
			node->offset = node->offset * fix;
		}
		// bring to the ground
		std::vector<btVector3> &list = mRoot->framePosition;
		float minY = list[0].y();
		for (int i = 1; i < list.size(); i++) {
			if (list[i].y() < minY)
				minY = list[i].y();
		}
		Log::debug("ground minY = %.2f", minY);
		for (int i = 0; i < list.size(); i++) {
			float y = list[i].y() - minY;
			list[i].setY(y * scale);
		}
	}

	virtual void nextFrame() {
		mCurrentFrame = ++mCurrentFrame >= mFrameCount? 0 : mCurrentFrame;
		Log::debug("frame = %d", mCurrentFrame);
		updateFrame(mRoot);
	}

	virtual void previousFrame() {
		mCurrentFrame = --mCurrentFrame < 0? mFrameCount - 1 : mCurrentFrame;
		Log::debug("frame = %d", mCurrentFrame);
		updateFrame(mRoot);
	}

	virtual void setMovementFactor(btVector3 movementFactor) {
		mMovementFactor = movementFactor;
	}

	virtual void start() {
		mStartTime = SDL_GetTicks();
	}

	virtual void stop() {
		mStartTime = 0;
	}

	virtual void paint(Camera* camera) {

		if (mStartTime) {
			Uint32 now = SDL_GetTicks();
			Uint32 timeSinceStart = now - mStartTime;
			Uint32 framesSinceStart = floor(timeSinceStart / mFrameTime);
			Uint32 frame = framesSinceStart % mFrameCount;
			mCurrentFrame = frame;
			updateFrame(mRoot);
		}

		glLineWidth(0.5);
		glPointSize(4.0);

		glColor3f(0.0, 0.0, 1.0);
		btVector3 v0 = mRoot->matrix.getVertex();
		glBegin(GL_POINTS);
		glVertex3f(v0.x(), v0.y(), v0.z());
		glEnd();

		glColor3f(1.0, 0.0, 0.0);
		paintNode(mRoot);
	}

	virtual void paintNode(Node* node) {

		if (node->parent) {
			btVector3 p0 = node->parent->matrix.getVertex();
			btVector3 v0 = node->matrix.getVertex();

			glBegin(GL_POINTS);
			glVertex3f(v0.x(), v0.y(), v0.z());
			glEnd();

			glBegin(GL_LINES);
			glVertex3f(p0.x(), p0.y(), p0.z());
			glVertex3f(v0.x(), v0.y(), v0.z());
			glEnd();
		}

		for (Node* child : node->children) {
			paintNode(child);
		}
	}
};

#endif
