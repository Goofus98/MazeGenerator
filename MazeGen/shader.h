#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <GL\glew.h>
class shader {
private:
	std::string vData{};
	std::string fData{};
	GLuint vvShader;
	GLuint ffShader;
	GLuint vffProgram;

	static inline std::string readFile(const std::string_view& src) {
		std::ifstream File;
		File.open(src.data(), std::ios_base::in);
		if (File) {
			std::string buffer((std::istreambuf_iterator<char>(File)),
				std::istreambuf_iterator<char>());
			std::cout << src.data() << "\n";
			std::cout << buffer.c_str() << "\n\n";
			return buffer;
		}
		else {
			std::cout << "Shader could not be loaded: " << src.data() << "\n";
			return std::string();
		}
	}
public:
	shader(const std::string_view& vertSource, const std::string_view& fragSource)
		: vData{ readFile(vertSource) }, fData{ readFile(fragSource) }
	{
		const char* pVertSource = vData.c_str();
		const char* pFragSource = fData.c_str();

		vvShader = glCreateShader(GL_VERTEX_SHADER);
		ffShader = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(vvShader, 1, &pVertSource, NULL);
		glShaderSource(ffShader, 1, &pFragSource, NULL);
		glCompileShader(vvShader);
		glCompileShader(ffShader);
		vffProgram = glCreateProgram();
		glAttachShader(vffProgram, vvShader);
		glAttachShader(vffProgram, ffShader);
		glLinkProgram(vffProgram);
	}

	const GLuint getProg() const  {
		return vffProgram;
	}
};