#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <glad/glad.h>
#undef NDEBUG
#include <cassert>


class GLShader
{
	int type;
	std::vector<std::string> src;

public:
	unsigned id;

	GLShader(int shaderType)
		: id(0), type(shaderType)
	{
		assert((type == GL_VERTEX_SHADER)
			|| (type == GL_FRAGMENT_SHADER)
			|| (type == GL_GEOMETRY_SHADER)
			|| (type == GL_COMPUTE_SHADER)
		);
	}

	GLShader& loadStr(char const* source)
	{
		assert(id == 0);
		src.push_back(std::string(source));
		return *this;
	}

	GLShader& loadFile(char const* filename)
	{
		assert(id == 0);
		std::ifstream fid;
		fid.open(filename, std::ios::ate);
		if (fid.is_open())
		{
			size_t len = static_cast<size_t>(fid.tellg());
			std::vector<char> obj(len + 1);
			fid.seekg(std::ios::beg);
			fid.read(obj.data(), len);
			fid.close();
			loadStr(obj.data());
		}
		return *this;
	}

	void compile()
	{
		assert(id == 0);
		GLsizei slen = static_cast<GLsizei>(src.size());
		std::vector<GLchar const*> ptr;
		std::vector<int> len;
		for (auto const& s : src)
		{
			ptr.push_back(s.c_str());
			len.push_back(static_cast<int>(s.length()));
		}
		id = glCreateShader(type);
		glShaderSource(id, slen, ptr.data(), len.data());
		glCompileShader(id);
		int err;
		glGetShaderiv(id, GL_COMPILE_STATUS, &err);
		if (!err)
		{
			char info[1024];
			glGetShaderInfoLog(id, 1023, NULL, info);
			fprintf(stderr,
				"===== GLShader %u type %d COMPILE ERROR %d =====\n%s\n",
				id, type, err, info);
			fflush(stderr);
		}
	}

	void release()
	{
		if (id)
			glDeleteShader(id);
		id = 0;
		src.clear();
	}

	~GLShader()
	{
		release();
	}
};


////////////////////////////////////////////////////////////


class GLProgram
{
	int done;

public:
	unsigned id;

	GLProgram()
		: id(0), done(0)
	{
		id = glCreateProgram();
	}

	~GLProgram()
	{
		if (id)
			glDeleteProgram(id);
	}

	GLProgram& attach(GLShader const& obj)
	{
		assert(done == 0);
		glAttachShader(id, obj.id);
		return *this;
	}

	GLProgram& link()
	{
		done = 1;
		glLinkProgram(id);
		int err;
		glGetProgramiv(id, GL_LINK_STATUS, &err);
		if (!err)
		{
			char info[1024];
			glGetProgramInfoLog(id, 1023, NULL, info);
			fprintf(stderr,
				"===== GLProgram %u LINK ERROR %d ==========\n%s\n",
				id, err, info);
			fflush(stderr);
		}
		return *this;
	}

	void use()
	{
		assert(done);
		glUseProgram(id);
	}

	unsigned uniform(char const* name)
	{
		return glGetUniformLocation(id, name);
	}
};

