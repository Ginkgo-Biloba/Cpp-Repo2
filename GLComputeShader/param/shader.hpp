#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <vector>
#include <fstream>
#include <glad/glad.h>
#undef NDEBUG
#include <cassert>


class GLShader
{
	int type;

public:
	unsigned id;

protected:
	void create(int objtype, char const* objstr)
	{
		id = 0;
		type = objtype;
		assert((type == GL_VERTEX_SHADER)
			|| (type == GL_FRAGMENT_SHADER)
			|| (type == GL_GEOMETRY_SHADER)
			|| (type == GL_COMPUTE_SHADER));

		id = glCreateShader(type);
		glShaderSource(id, 1, &objstr, NULL);
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

public:
	GLShader(int objtype, char const* objstr)
		: id(0), type(0)
	{
		create(objtype, objstr);
	}

	GLShader(int objtype, char const* objfile, int)
		: id(0), type(0)
	{
		std::vector<char> objstr;
		std::ifstream fid;
		fid.open(objfile, std::ios::ate);
		if (!fid.is_open())
			return;

		size_t len = static_cast<size_t>(fid.tellg());
		objstr.resize(len + 1);
		fid.seekg(std::ios::beg);
		fid.read(objstr.data(), len);
		fid.close();

		create(objtype, objstr.data());
	}

	~GLShader()
	{
		if (id)
			glDeleteShader(id);
	}
};


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

	void attach(GLShader obj)
	{
		assert(!done);
		glAttachShader(id, obj.id);
	}

	void link()
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


