#ifndef COLLISIONSHAPEFACTORY_H_
#define COLLISIONSHAPEFACTORY_H_

#include <BulletCollision/CollisionShapes/btConvexHullShape.h>


class CollisionShapeFactory
{
public:
	static std::unique_ptr<btConvexHullShape> createCenteredTrapezium(
		float halfHeight,
		float baseHalfWidth,
		float baseHalfLength,
		float topHalfWidth,
		float topHalfLength)
	{
		auto hullShape = std::make_unique<btConvexHullShape>();
		hullShape->addPoint(btVector3(baseHalfWidth, -halfHeight, baseHalfLength));
		hullShape->addPoint(btVector3(-baseHalfWidth, -halfHeight, baseHalfLength));
		hullShape->addPoint(btVector3(-baseHalfWidth, -halfHeight, -baseHalfLength));
		hullShape->addPoint(btVector3(topHalfWidth, halfHeight, topHalfLength));
		hullShape->addPoint(btVector3(topHalfWidth, halfHeight, -topHalfLength));
		hullShape->addPoint(btVector3(-topHalfWidth, halfHeight, -topHalfLength));
		hullShape->addPoint(btVector3(-topHalfWidth, halfHeight, topHalfLength));
		hullShape->addPoint(btVector3(baseHalfWidth, -halfHeight, -baseHalfLength));
		return hullShape;
	}

	static std::unique_ptr<btConvexHullShape> createAlignedTrapezium(
		float halfHeight,
		float baseHalfWidth,
		float baseHalfLength,
		float topHalfWidth,
		float topHalfLength)
	{
		float diff = baseHalfLength - topHalfLength;
		auto hullShape = std::make_unique<btConvexHullShape>();
		hullShape->addPoint(btVector3(baseHalfWidth, -halfHeight, baseHalfLength));
		hullShape->addPoint(btVector3(-baseHalfWidth, -halfHeight, baseHalfLength));
		hullShape->addPoint(btVector3(-baseHalfWidth, -halfHeight, -baseHalfLength));
		hullShape->addPoint(btVector3(topHalfWidth, halfHeight, topHalfLength - diff));
		hullShape->addPoint(btVector3(topHalfWidth, halfHeight, -topHalfLength - diff));
		hullShape->addPoint(btVector3(-topHalfWidth, halfHeight, -topHalfLength - diff));
		hullShape->addPoint(btVector3(-topHalfWidth, halfHeight, topHalfLength - diff));
		hullShape->addPoint(btVector3(baseHalfWidth, -halfHeight, -baseHalfLength));
		return hullShape;
	}

};


#endif /* COLLISIONSHAPEFACTORY_H_ */
