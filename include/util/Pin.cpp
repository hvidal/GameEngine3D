#include "Pin.h"
#include "ShaderUtils.h"


static constexpr const char* vs[] = {
	"attribute vec3 vertex;"
	"attribute vec3 normal;"

	"uniform mat4 V;"
	"uniform mat4 P;"

	"uniform vec3 position;"
	"uniform vec3 lightPosition;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	,ShaderUtils::TO_MAT3,
	""
	,ShaderUtils::TO_PLANET_SURFACE,

	"void main() {"
		"vec3 worldV = position + toPlanetSurface(position, vertex);"
		"eyeN = toMat3(V) * toPlanetSurface(position, normal);"
		"eyeL = V * vec4(lightPosition, 1.0);"
		"eyeV = V * vec4(worldV, 1.0);"
		"gl_Position = P * eyeV;"
	"}"
};

static constexpr const char* fs[] = {
	"uniform vec3 color;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL, vec4 c) {"
		"vec3 n = normalize(eyeN);"
		"vec3 s = normalize(eyeL - eyeV);"
		"vec3 v = normalize(-eyeV);"

		"float dotSN = dot(s, n);"
		"float shading = max(dotSN, 0.0);"
		"float occlusion = dotSN < 0.0? 1.0 - abs(dotSN) : 1.0;"
		"return (0.5 * c) + c * shading * occlusion;"
	"}"
	"void main() {"
		"vec4 c = vec4(color, 1.0);"
		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz, c);"
		"gl_FragColor.a = 1.0;"
	"}"
};


static constexpr const char* vs_Instanced[] = {
	"attribute vec3 vertex;"
	"attribute vec3 normal;"
	"attribute vec3 position;"

	"uniform mat4 V;"
	"uniform mat4 P;"

	"uniform vec3 lightPosition;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	,ShaderUtils::TO_MAT3,
	""
	,ShaderUtils::TO_PLANET_SURFACE,

	"void main() {"
		"vec3 worldV = position + toPlanetSurface(position, vertex);"
		"eyeN = toMat3(V) * toPlanetSurface(position, normal);"
		"eyeL = V * vec4(lightPosition, 1.0);"
		"eyeV = V * vec4(worldV, 1.0);"
		"gl_Position = P * eyeV;"
	"}"
};

//-----------------------------------------------------------------------------

static constexpr const int gIndiceCount = 24;

static std::vector<unsigned int> gIndices;
static std::vector<btVector3> gVertices;
static std::vector<btVector3> gNormals;

Pin::Pin() {
	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "normal");
	mShader->link();

	mShaderInstanced = std::make_unique<Shader>(vs_Instanced, fs);
	mShaderInstanced->bindAttribute(0, "vertex");
	mShaderInstanced->bindAttribute(1, "normal");
	mShaderInstanced->bindAttribute(2, "position");
	mShaderInstanced->link();

	constexpr const static float SIZE = 1.5f;
	btVector3 v[] = {
		btVector3(0.f, 0.f, 0.f),
		btVector3(SIZE, SIZE, SIZE),
		btVector3(SIZE, SIZE, -SIZE),
		btVector3(-SIZE, SIZE, -SIZE),
		btVector3(-SIZE, SIZE, SIZE),
		btVector3(0.f, 2.f * SIZE, 0.f)
	};

	constexpr unsigned int rawVertexIndices[] = {
		0,2,1,
		0,3,2,
		0,4,3,
		0,1,4,
		1,2,5,
		2,3,5,
		3,4,5,
		4,1,5
	};

	gVertices.reserve(gIndiceCount);
	gNormals.reserve(gIndiceCount);

	for (int i = 0; i < gIndiceCount; i++) {
		unsigned int raw = rawVertexIndices[i];
		gVertices.push_back(v[raw]);

		int row = 3 * (i / 3);
		btVector3 a = v[rawVertexIndices[row]];
		btVector3 b = v[rawVertexIndices[row+1]];
		btVector3 c = v[rawVertexIndices[row+2]];
		btVector3 n = (a + b + c).normalized();

		gNormals.push_back(n);
		gIndices.push_back(i);
	}

	glGenBuffers(1, &mVbo);
	glGenBuffers(1, &mNbo);
	glGenBuffers(1, &mIbo);

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, gVertices.size() * sizeof(btVector3), &gVertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, mNbo);
	glBufferData(GL_ARRAY_BUFFER, gNormals.size() * sizeof(btVector3), &gNormals[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, gIndiceCount * sizeof(unsigned int), &gIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


Pin::~Pin() {
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mNbo);
	glDeleteBuffers(1, &mIbo);
}


void Pin::render(const btVector3& point, const ICamera* camera, const btVector3& lightPosition, const btVector3& color) {
	mShader->run();
	camera->setMatrices(mShader.get());
	mShader->set("position", point);
	mShader->set("lightPosition", lightPosition);
	mShader->set("color", color);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, mNbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glDrawElements(GL_TRIANGLES, gIndiceCount, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Shader::stop();
}


void Pin::render(const std::vector<btVector3>& points, const ICamera* camera, const btVector3& lightPosition, const btVector3& color) {
	mShaderInstanced->run();
	camera->setMatrices(mShaderInstanced.get());
	mShaderInstanced->set("lightPosition", lightPosition);
	mShaderInstanced->set("color", color);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribDivisor(0, 0); // vertices : always reuse the same vertices	-> 0
	glVertexAttribDivisor(1, 0); // normals : always reuse the same uv 			-> 0
	glVertexAttribDivisor(2, 1); // positions : one per quad (its center)		-> 1

	// Shared: Vertices
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);
	// Shared: Normals
	glBindBuffer(GL_ARRAY_BUFFER, mNbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);
	// Shared: Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);

	// Positions
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(btVector3), &points[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);
	glDrawElementsInstanced(GL_TRIANGLES, gIndiceCount, GL_UNSIGNED_INT, 0, points.size());

	glVertexAttribDivisor(2, 0); // disable

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Shader::stop();
}








