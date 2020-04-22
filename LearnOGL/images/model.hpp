#ifndef MODEL_HPP
#define MODEL_HPP

#include <vector>
#include <string>
#include <opencv2/highgui.hpp>
#include "shader.hpp"

using std::vector;
using std::string;


struct Vertex
{
	glm::vec3 Position, Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent, Bitangent;
};


struct Texture
{
	unsigned id;
	string type, path;
};


/****************************** Mesh ******************************/

class Mesh
{
public:
	vector<Vertex> vertices;
	vector<unsigned> indices;
	vector<Texture> textures;
	unsigned VAO;

protected:
	unsigned VBO, EBO;
	void setupMesh()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		glBindVertexArray(VAO);

		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array 
		// which again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), 0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
			reinterpret_cast<void*>(offsetof(Vertex, Normal)));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
			reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
			reinterpret_cast<void*>(offsetof(Vertex, Tangent)));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
			reinterpret_cast<void*>(offsetof(Vertex, Bitangent)));
	}

public:
	Mesh(vector<Vertex>& vertices, vector<unsigned>& indices, vector<Texture>& textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		// now that we have all the required data
		// set the vertex buffers and its attribute pointers.
		setupMesh();
	}

	// render the mesh
	void Draw(Shader shader)
	{
		// bind appropriate textures
		unsigned diffuseNr = 1, specularNr = 1, normalNr = 1, heightNr = 1;
		int number, len = static_cast<int>(textures.size());
		char buf[1 << 10];

		for (int i = 0; i < len; ++i)
		{
			// active proper texture unit before binding
			glActiveTexture(GL_TEXTURE0 + i);
			// retrieve texture number (the N in diffuse_textureN)
			std::string name = textures[i].type;
			if (name == "texture_diffuse")
				number = diffuseNr++;
			else if (name == "texture_specular")
				number = specularNr++;
			else if (name == "texture_normal")
				number = normalNr++;
			else if (name == "texture_height")
				number = heightNr++;

			// now set the sampler to the correct texture unit
			snprintf(buf, sizeof(buf), "%s%d", name.c_str(), i);
			glUniform1i(glGetUniformLocation(shader.ID, buf), i);
			// and finally bind the texture
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}

		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
};


/****************************** Model ******************************/

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


unsigned TextureFromFile(const char *path, string const& directory,
	bool gamma = false)
{
	string filename = path + directory + '/' + filename;

	unsigned textureID;
	glGenTextures(1, &textureID);

	cv::Mat im = imread(filename, cv::IMREAD_UNCHANGED);
	if (!(im.empty()))
	{
		GLenum format;
		if (im.channels() == 1)
			format = GL_RED;
		else if (im.channels() == 3)
			format = GL_BGR;
		else if (im.channels() == 4)
			format = GL_BGRA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, im.cols, im.rows, 0, format, GL_UNSIGNED_BYTE, im.data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
		std::cout << "Texture failed to load at path: " << path << std::endl;

	return textureID;
}


class Model
{
	// stores all the textures loaded so far
	// optimization to make sure textures aren't loaded more than once.
	vector<Texture> textload;
	vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;
	
	vector<Texture> loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, string const& typeName)
	{
		vector<Texture> T;
		for (unsigned i = 0; i < mat->GetTextureCount(type); ++i)
		{
			aiString ais;
			mat->GetTexture(type, i, &ais);
			// check if texture was loaded before 
			// and if so,continue to next iteration: skip loading a new texture
			unsigned k = 0;
			for (; k < textload.size(); ++k)
				if (!strcmp(textload[k].path.c_str(), ais.C_Str()))
				{
					T.push_back(textload[k]);
					break;
				}
			if (k == textload.size())
			{
				Texture t;
				t.id = TextureFromFile(ais.C_Str(), directory);
				t.type = typeName;
				t.path = ais.C_Str();
				T.push_back(t);
				textload.push_back(t);
			}
		}
		return T;
	}


	Mesh processMesh(aiMesh* mesh, aiScene const* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned> indices;
		vector<Texture> textures;

		// Walk through each of the mesh's vertices
		for (unsigned i = 0; i < mesh->mNumVertices; ++i)
		{
			// we declare a placeholder vector since assimp uses its own vector class 
			// that doesn't directly convert to glm's vec3 class
			Vertex vertex;
			vertex.Position = glm::vec3(
				mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex.Normal = glm::vec3(
				mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			// does the mesh contain texture coordinates?
			if (mesh->mTextureCoords[0])
			{
				// a vertex can contain up to 8 different texture coordinates
				// We thus make the assumption that we won't use models 
				// where a vertex can have multiple texture coordinates 
				// so we always take the first set (0).
				vertex.TexCoords = glm::vec2(
					mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else
				vertex.TexCoords = glm::vec2(0, 0);
			vertex.Tangent = glm::vec3(
				mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			vertex.Bitangent = glm::vec3(
				mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			vertices.push_back(vertex);
		}

		// now wak through each of the mesh's faces (a face is a mesh its triangle) 
		// and retrieve the corresponding vertex indices.
		for (unsigned i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned k = 0; k < face.mNumIndices; ++k)
				indices.push_back(face.mIndices[k]);
		}

		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders
		// Each diffuse texture should be named as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		// 1. diffuse maps
		vector<Texture> dif = loadMaterialTextures(material,
			aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), dif.begin(), dif.end());
		// 2. specular maps
		vector<Texture> spe = loadMaterialTextures(material,
			aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), spe.begin(), spe.end());
		// 3. normal maps
		vector<Texture> nor = loadMaterialTextures(material,
			aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), nor.begin(), nor.end());
		// 4. height maps
		vector<Texture> hei = loadMaterialTextures(material,
			aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), hei.begin(), hei.end());

		// return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures);
	}


	void processNode(aiNode* node, aiScene const* scene)
	{
		// process each mesh located at the current node
		for (unsigned i = 0; i < node->mNumMeshes; ++i)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* cur = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(cur, scene));
		}
		// after we've processed all of the meshes (if any),
		// we then recursively process each of the children nodes
		for (unsigned i = 0; i < node->mNumChildren; ++i)
			processNode(node->mChildren[i], scene);
	}


	void loadModel(char const* path)
	{
		Assimp::Importer importer;
		aiScene const* scene = importer.ReadFile(path, 
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !(scene->mRootNode))
		{
			std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}
		directory = std::string(path);
		if (directory.find_last_of('/') != std::string::npos)
		{
			while (directory.back() != '/')
				directory.pop_back();
			directory.pop_back();
		}
		processNode(scene->mRootNode, scene);
	}

public:
	Model(char const* path, bool g = false)
	{
		gammaCorrection = g;
		loadModel(path); 
	}

	void Draw(Shader shader)
	{
		size_t len = meshes.size();
		for (size_t i = 0; i < len; ++i)
			meshes[i].Draw(shader);
	}
};


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/


/******************************  ******************************/





#endif // !AMASH_HPP

