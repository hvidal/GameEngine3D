cmake_minimum_required(VERSION 3.3)
project(gamedev3d)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
SET(CMAKE_CXX_LINK_FLAGS "-v -lBulletDynamics -lBulletCollision -lLinearMath -framework OpenGL -framework SDL2 -framework SDL2_ttf -framework SDL2_image")

include_directories(/Users/hugo/Gamedev/bullet-2.81-rev2613/src)
link_directories(/Users/hugo/Gamedev/bullet-2.81-rev2613/src/.libs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/Users/hugo/work/gamedev3d/Release/")

set(SOURCE_FILES
		src/Main.cpp

		include/app/Interfaces.h

		include/app/Window.h
		include/app/Window.cpp
		include/app/Application.h
		include/app/Application.cpp
		include/app/Serializer.cpp
		include/app/Serializer.h

		include/camera/Camera.h
		include/camera/Camera.cpp
		include/camera/WASDCamera.h
		include/camera/WASDCamera.cpp
		include/camera/ChaseCamera.h
		include/camera/ChaseCamera.cpp

		include/extra/FPSCounter.h
		include/extra/FPSCounter.cpp
		include/extra/MouseShootFeature.h
		include/extra/MouseShootFeature.cpp

		include/scene/SceneObject.h
		include/scene/SceneObject.cpp
		include/scene/Renderer2d.h
		include/scene/Renderer2d.cpp

		include/scene/planet/Planet.h
		include/scene/planet/Planet.cpp
		include/scene/planet/PlanetFace.h
		include/scene/planet/PlanetFace.cpp
		include/scene/planet/PlanetPage.h
		include/scene/planet/PlanetPage.cpp
		include/scene/planet/SurfaceReflection.h
		include/scene/planet/SurfaceReflection.cpp
		include/scene/planet/IPlanetExternalObject.h

		include/scene/road/Road.h
		include/scene/road/Road.cpp
		include/scene/road/CityBlock.cpp
		include/scene/road/CityBlock.h

		include/scene/vegetation/Tree.h
		include/scene/vegetation/Tree.cpp
		include/scene/vegetation/Plant.h
		include/scene/vegetation/Plant.cpp
		include/scene/vegetation/Grass.h
		include/scene/vegetation/Grass.cpp

		include/scene/sky/Clouds.h
		include/scene/sky/Clouds.cpp
		include/scene/sky/Sky.h
		include/scene/sky/Sky.cpp
		include/scene/sky/SkyShader.h

		include/scene/shape/CubeShape.h
		include/scene/shape/SphereShape.h
		include/scene/shape/SphereShape.cpp

		include/scene/vehicle/FourWheels.h
		include/scene/vehicle/FourWheels.cpp
        include/scene/vehicle/KeyboardVehicleControl.h
        include/scene/vehicle/KeyboardVehicleControl.cpp
		include/scene/vehicle/Vehicle.h

		include/scene/character/OBJCharacter.h
		include/scene/character/OBJCharacter.cpp
		include/scene/character/KeyboardCharacterControl.h
		include/scene/character/KeyboardCharacterControl.cpp

		include/util/math/FieldOfView.h
		include/util/math/FieldOfView.cpp
		include/util/math/Plane.cpp
		include/util/math/Plane.h
		include/util/math/Line.h
		include/util/math/Triangle.cpp
		include/util/math/Triangle.h
		include/util/math/Matrix4x4.h
		include/util/math/Matrix4x4.cpp

		include/util/ShaderUtils.h
		include/util/BvhAnimation.h
		include/util/CollisionShapeFactory.h
		include/util/FrameBuffer.h
		include/util/FrameBuffer.cpp
		include/util/GLShapeRenderer.h
		include/util/GLShapeRenderer.cpp
		include/util/IoUtils.h
		include/util/IoUtils.cpp
		include/util/Log.h
		include/util/ModelOBJ.cpp
		include/util/ModelOBJ.h
		include/util/ModelOBJRenderer.h
		include/util/ModelOBJRenderer.cpp
		include/util/OBJAnimation.h
		include/util/OBJAnimation.cpp
		include/util/PerlinNoise.h
		include/util/Shader.h
		include/util/Shader.cpp
		include/util/ShaderNoise.h
		include/util/ShadowMap.h
		include/util/ShadowMap.cpp
		include/util/SimplexNoise.h
		include/util/StringUtils.h
		include/util/Texture.h
		include/util/Texture.cpp
		include/util/Pin.h
		include/util/Pin.cpp
		include/util/ContainerUtils.h
		include/util/GLUtils.h
		include/util/PhysicsBody.h
		include/util/PhysicsBody.cpp
		)

add_executable(gamedev3d ${SOURCE_FILES})