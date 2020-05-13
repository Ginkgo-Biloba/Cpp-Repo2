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
	int type, ncd;
	std::string code;

public:
	unsigned id;

	GLShader(int shaderType)
		: type(shaderType), ncd(0), id(0)
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
		char buffer[64] = { 0 };
		snprintf(buffer, sizeof(buffer),
			"\n\n/*_____----- %d -----_____*/\n\n",
			++ncd);
		code.append(source);
		code.append(buffer);
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
		id = glCreateShader(type);
		char const* source = code.c_str();
		glShaderSource(id, 1, &source, NULL);
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
		code.clear();
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
		: done(0), id(0)
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


void savePGM(GLubyte const* buf, int width, int height, int frame)
{
	char path[256];
	snprintf(path, 256, R"~(G:\Gallery\%04d.ppm)~", frame);
	FILE* fid = fopen(path, "wb");
	if (!fid)
	{
		if (frame == 0)
		{
			printf("can not open %s for write\n", path);
			perror("create file error");
		}
		return;
	}
	snprintf(path, 256, "P6\n%d %d\n255\n", width, height);
	fputs(path, fid);
	fwrite(buf, sizeof(GLubyte), width * height * 3, fid);
	fclose(fid);
}



