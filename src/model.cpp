
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <numeric>

struct FaceElement
{
	unsigned int vertexIndex;
	unsigned int textureCoordIndex;
	unsigned int normalIndex;
};

struct Face
{
	FaceElement element[4];
};

struct ObjModel
{
	std::vector < glm::vec4 > vertices;
	std::vector < glm::vec2 > textureCoords;
	std::vector < Face > faces;
};

struct ModelConfig
{
	glm::vec3 eyePosition;
	glm::vec3 eyeTarget;
	glm::vec3 eyeUpDirection;

	std::map<std::string, glm::vec3> origins;

	std::map<std::string, float> attachmentScales;
};

inline void DropLine(std::istream &stream)
{
	stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline glm::vec3 ReadVec3(std::istream &input)
{
	glm::vec3 vec;
	input >> vec.x >> vec.y >> vec.z;
	return vec;
}

inline float ReadFloat(std::istream &input)
{
	float value;
	input >> value;
	return value;
}

std::map<std::string, ObjModel> LoadObjModel(std::string filename, float textureWidth, float textureHeight)
{
	std::map<std::string, ObjModel> ret;
	ObjModel* current = nullptr;
	std::string token;
	std::ifstream input(filename, std::ios::in);
	do {
		input >> token;
		if (token == "g")
		{
			input >> token;
			current = &(ret[token]);
		}
		if (current == nullptr)
		{
			DropLine(input);
			continue;
		}
		if (token == "v")
		{
			glm::vec3 point;
			input >> point.x >> point.y >> point.z;
			current->vertices.push_back(glm::vec4(point,1.0f));
		}
		else if (token == "vt")
		{
			glm::vec2 point;
			input >> point.x >> point.y;
			point.x /= textureWidth;
			point.y /= textureHeight;
			point.y = 1.0f - point.y;
			current->textureCoords.push_back(point);
		}
		else if (token == "f")
		{
			Face face;
			for (size_t i = 0; i < 4; ++i)
			{
				input >> face.element[i].vertexIndex;
				input.ignore();  // ignores '/'
				input >> face.element[i].textureCoordIndex;
			}
			current->faces.push_back(face);
		}
		DropLine(input);
	} while (!input.eof());
	input.close();
	return ret;
}

ModelConfig LoadModelConfig(std::string filename)
{
	std::ifstream input(filename, std::ios::in);
	ModelConfig ret;
	std::string token;
	do {
		input >> token;
		if (token == "headOrigin")
			ret.origins["Head"] = ReadVec3(input);
		else if (token == "bodyOrigin")
			ret.origins["Body"] = ReadVec3(input);
		else if (token == "leftArmOrigin")
			ret.origins["LeftArm"] = ReadVec3(input);
		else if (token == "rightArmOrigin")
			ret.origins["RightArm"] = ReadVec3(input);
		else if (token == "leftLegOrigin")
			ret.origins["LeftLeg"] = ReadVec3(input);
		else if (token == "rightLegOrigin")
			ret.origins["RightLeg"] = ReadVec3(input);
		else if (token == "headAttachmentScale")
			ret.attachmentScales["Head"] = ReadFloat(input);
		else if (token == "bodyAttachmentScale")
			ret.attachmentScales["Body"] = ReadFloat(input);
		else if (token == "leftArmAttachmentScale")
			ret.attachmentScales["LeftArm"] = ReadFloat(input);
		else if (token == "rightArmAttachmentScale")
			ret.attachmentScales["RightArm"] = ReadFloat(input);
		else if (token == "leftLegAttachmentScale")
			ret.attachmentScales["LeftLeg"] = ReadFloat(input);
		else if (token == "rightLegAttachmentScale")
			ret.attachmentScales["RightLeg"] = ReadFloat(input);
		else if (token == "eyePosition")
			ret.eyePosition = ReadVec3(input);
		else if (token == "eyeTarget")
			ret.eyeTarget = ReadVec3(input);
		else if (token == "eyeUpDirection")
			ret.eyeUpDirection = ReadVec3(input);
		DropLine(input);
	} while (!input.eof());
	input.close();
	return ret;
}