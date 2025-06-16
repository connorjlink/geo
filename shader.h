#ifndef GEO_SHADER_H
#define GEO_SHADER_H

#include <filesystem>
#include <fstream>
#include <print>

namespace geo
{
	class Shader
	{
	private:
		const GLuint _type;
		const GLuint _program_id;
		GLuint _shader_id;

	public:
		Shader(const GLuint type, const GLuint program_id, const std::string& filepath)
			: _type{ type }, _program_id{ program_id }
		{
			std::fstream file(filepath.data());

			if (!file.good())
			{
				PANIC("Could not read shader source file");
				panic("failed to read something");
			}

			auto size = std::filesystem::file_size(filepath);

			std::string code(size, '\0');
			code.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

			_shader_id = glCreateShader(_type);
			auto data = code.data();
			glShaderSource(_shader_id, 1, &data, NULL);
			glCompileShader(_shader_id);
			glAttachShader(_program_id, _shader_id);
		}

	public:
		void release() const
		{
			glDetachShader(_program_id, _shader_id);
			glDeleteShader(_shader_id);
		}
	};

	class ShaderFactory
	{
	private:
		const GLuint _program_id;
		std::vector<Shader> _shaders;

	public:
		ShaderFactory(const GLuint program_id)
			: _program_id{ program_id }, _shaders{}
		{
		}

	public:
		void compile_shader(const GLuint type, const std::string& filepath)
		{
			_shaders.emplace_back(Shader(type, _program_id, filepath));
		}

	public:
		void link()
		{
			glLinkProgram(_program_id);

			for (const auto& shader : _shaders)
			{
				shader.release();
			}
		}
	};

	class ShaderProgram
	{
	private:
		const GLuint _program_id;

	public:
		void use() const
		{
			glUseProgram(_program_id);
		}

		const GLuint locate_uniform(const std::string& identifier)
		{
			return glGetUniformLocation(_program_id, identifier.data());
		}

		void upload_matrix(const fx::mat4& matrix, const std::string& identifier)
		{
			auto matrix_id = locate_uniform(identifier);
			glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &matrix[0][0]);
		}

	public:
		ShaderProgram(const std::string& partial_filepath)
			: _program_id{ glCreateProgram() }
		{
			ShaderFactory factory{ _program_id };
			factory.compile_shader(GL_VERTEX_SHADER, partial_filepath + ".vertex.glsl");
			factory.compile_shader(GL_FRAGMENT_SHADER, partial_filepath + ".fragment.glsl");
			factory.link();
		}

		~ShaderProgram()
		{
			glDeleteProgram(_program_id);
		}
	};
}

#endif
