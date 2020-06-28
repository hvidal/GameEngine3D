#ifndef INCLUDE_CUBE_SHAPE_H_
#define INCLUDE_CUBE_SHAPE_H_

#include "btBulletDynamicsCommon.h"
#include "../SceneObject.h"
#include "../../util/ModelOBJRenderer.h"
#include "../../util/GLShapeRenderer.h"

class CubeShape: public SceneObject
{
private:
	float mMass;
	float mSide;

	ModelOBJRenderer* mCubeModel;
	GLShapeRenderer* mShapeRenderer;

public:
	CubeShape(float mass, float side, std::string filename, btDynamicsWorld* dynamicsWorld) :
		SceneObject(dynamicsWorld),
		mMass(mass),
		mSide(side)
	{
		mCubeModel = ModelOBJRendererCache::get(filename);
//		mCubeModel->getModel()->scale(mSide, mSide, mSide);
		mShapeRenderer = new GLShapeRenderer();
	}

	virtual ~CubeShape() {
		delete mShapeRenderer;
	}

	void initPhysics(btTransform transform) override;
	void renderOpaque(const ICamera* camera, const ILight* light, const ShadowMap* shadowMap, const GameState& gameState) override;
};

void CubeShape::initPhysics(btTransform transform)
{
	float half = mSide * 0.5f;
	btConvexInternalShape* shape = new btBoxShape(btVector3(half, half, half));
	this->localCreateRigidBody(mMass, transform, shape);
}

void CubeShape::renderOpaque(const ICamera* camera, const ILight* light, const ShadowMap* shadowMap, const GameState& gameState)
{
	btVector3 center = mRigidBody->getCenterOfMassPosition();
	if (!camera->isVisible(center))
		return;

	btScalar m[16];
	loadOpenGLMatrix(m);
	if (gameState.isDebugging)
		mShapeRenderer->render(m, light, mRigidBody->getCollisionShape(), true);
	else
		mCubeModel->render(camera, light, shadowMap, m, true);
}

#endif