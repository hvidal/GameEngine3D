#ifndef GAMEDEV3D_IPLANETEXTERNALOBJECT_H
#define GAMEDEV3D_IPLANETEXTERNALOBJECT_H

#include "../../app/Interfaces.h"


class IPlanetExternalObject: public ISerializable {
public:
	virtual ~IPlanetExternalObject(){}

	virtual void renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const GameState& gameState) {};
	virtual void renderTranslucent(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const GameState& gameState) {};
};


#endif
