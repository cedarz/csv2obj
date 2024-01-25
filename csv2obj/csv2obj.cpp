#include <istream>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <array>

void printHelp()
{
	std::cout << "Using: csv2obj input.csv" << std::endl;
}

using FloatVector = std::array<float, 3>;

struct ObjFile
{
	std::vector<uint32_t> indices;
	std::vector<FloatVector> vertices;
	std::vector<FloatVector> texCoords;
};

void cleanAndSplit(char buffer[], std::vector<std::string>& values)
{
	char* end = std::remove(buffer, buffer + strlen(buffer), ',');
	*end = 0;

	std::istringstream iss(buffer);
	values = std::vector<std::string>(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
}

//VTX, IDX, POSITION.x, POSITION.y, POSITION.z, TANGENT.x, TANGENT.y, TANGENT.z, NORMAL.x, NORMAL.y, NORMAL.z, TEXCOORD.x, TEXCOORD.y
uint64_t vertex_id = std::hash<std::string>().operator()("VTX");
uint64_t index_id = std::hash<std::string>().operator()("IDX");
uint64_t position_x = std::hash<std::string>().operator()("POSITION.x");
uint64_t position_y = std::hash<std::string>().operator()("POSITION.y");
uint64_t position_z = std::hash<std::string>().operator()("POSITION.z");
uint64_t texcoord_x = std::hash<std::string>().operator()("TEXCOORD.x");
uint64_t texcoord_y = std::hash<std::string>().operator()("TEXCOORD.y");


void distributeValues(const std::vector<uint64_t>& header, const std::vector<std::string>& values, ObjFile& objFile)
{
	assert(header.size() == values.size());

	uint32_t curIndex = -1;
	for (int loc = 0; loc < header.size(); ++loc)
	{
		if (header[loc] == index_id)
		{
			curIndex = std::stoi(values[loc]);
			objFile.indices.push_back(curIndex + 1); // obj index start from 1 not 0
			if (objFile.vertices.size() < curIndex + 1) {
				objFile.vertices.resize(curIndex + 1);
				objFile.texCoords.resize(curIndex + 1);
			}

			break;
		}
	}

	if (curIndex == -1)
	{
		curIndex = objFile.vertices.size();
		objFile.indices.push_back(curIndex + 1);
		objFile.vertices.resize(curIndex + 1);
		objFile.texCoords.resize(curIndex + 1);
	}

	for (int loc = 0; loc < header.size(); ++loc)
	{
		uint64_t h = header[loc];

		if (h == position_x)
		{
			objFile.vertices[curIndex][0] = std::stof(values[loc]);
		}
		else if (h == position_y)
		{
			objFile.vertices[curIndex][1] = std::stof(values[loc]);
		}
		else if (h == position_z)
		{
			objFile.vertices[curIndex][2] = std::stof(values[loc]);
		}
		else if (h == texcoord_x)
		{
			objFile.texCoords[curIndex][0] = std::stof(values[loc]);
		}
		else if (h == texcoord_y)
		{
			objFile.texCoords[curIndex][1] = std::stof(values[loc]);
		}
	}
}

void writeObj(const ObjFile& obj, const char* outFileName)
{
	std::ofstream fOut(outFileName);
	for (const auto& v : obj.vertices)
		fOut << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
	for (const auto& v : obj.texCoords)
		fOut << "vt " << v[0] << " " << v[1] << std::endl;
	fOut << "g default" << std::endl;

	size_t trianglesCount = obj.indices.size() / 3;
	for (size_t i = 0; i < trianglesCount; ++i)
	{
		fOut << "f " << obj.indices[3 * i + 0] << "/" << obj.indices[3 * i + 0] << " " 
					 << obj.indices[3 * i + 1] << "/" << obj.indices[3 * i + 1] << " " 
					 << obj.indices[3 * i + 2] << "/" << obj.indices[3 * i + 2] << std::endl;
	}

	fOut.close();
}

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		printHelp();
		return 1;
	}

	size_t fileNameSize = strlen(argv[1]);
	if (strcmp(argv[1] + fileNameSize - 4, ".csv"))
	{
		printHelp();
		return 2;
	}

	ObjFile output;

	char outputFileName[2048] = {};
	snprintf(outputFileName, sizeof(outputFileName), "%s", argv[1]);
	snprintf(outputFileName + fileNameSize - 4, 5, ".obj");

	char buffer[2048] = {};
	const char* inputFileName = argv[1];
	std::ifstream fIn(inputFileName);
	fIn.getline(buffer, sizeof(buffer));

	std::vector<std::string> headerValues;
	cleanAndSplit(buffer, headerValues);
	std::vector<uint64_t> hashes;
	for (const std::string& h : headerValues)
		hashes.emplace_back(std::hash<std::string>().operator()(h));

	while (!fIn.eof())
	{
		fIn.getline(buffer, sizeof(buffer));
		if (strlen(buffer) == 0)
			break;

		std::vector<std::string> values;
		cleanAndSplit(buffer, values);
		distributeValues(hashes, values, output);
	}

	fIn.close();

	writeObj(output, outputFileName);

	return 0;
}
