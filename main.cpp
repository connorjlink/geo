#include <iostream>
#include <format>
#include <print>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>

#include <array>
#include <cstdlib>
#include <cmath>

#include <Windows.h>

#include "glad.h"
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_SIMD_AVX2
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PANIC(x) std::println(std::cerr, x); std::cin.get(); std::exit(EXIT_FAILURE)

class shader
{
private:
	const GLuint _type;
	const GLuint _program_id;
	GLuint _shader_id;

public:
	shader(const GLuint type, const GLuint program_id, std::string_view filepath)
		: _type{ type }, _program_id{ program_id }
	{
		std::fstream file(filepath.data());

		if (!file.good())
		{
			PANIC("Could not read shader source file");
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

class shader_factory
{
private:
	const GLuint _program_id;
	std::vector<shader> _shaders;

public:
	shader_factory(const GLuint program_id)
		: _program_id{ program_id }, _shaders{}
	{
	}

public:
	void compile_shader(const GLuint type, std::string_view filepath)
	{
		_shaders.emplace_back(shader(type, _program_id, filepath));
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

class shader_program
{
private:
	const GLuint _program_id;

public:
	void use() const
	{
		glUseProgram(_program_id);
	}

	const GLuint locate_uniform(std::string_view identifier)
	{
		return glGetUniformLocation(_program_id, identifier.data());
	}

	void upload_matrix(const glm::mat4& matrix, std::string_view identifier)
	{
		auto matrix_id = locate_uniform(identifier);
		glUniformMatrix4fv(matrix_id, 1, GL_FALSE, glm::value_ptr(matrix));
	}

public:
	shader_program(std::string partial_filepath)
		: _program_id { glCreateProgram() }
	{
		shader_factory factory{ _program_id };
		factory.compile_shader(GL_VERTEX_SHADER, partial_filepath + ".vertex.glsl");
		factory.compile_shader(GL_FRAGMENT_SHADER, partial_filepath + ".fragment.glsl");
		factory.link();
	}

	~shader_program()
	{
		glDeleteProgram(_program_id);
	}
};

// TODO: let's use attribute string names instead of IDs
GLuint _attribute_id = 0;

template<typename T>
class buffer
{
private:
	const GLuint _type;
	GLuint _buffer_id;

private:
	std::vector<T>& _data;

public:
	void static_bind()
	{
		const auto stride = sizeof(std::remove_reference_t<decltype(_data)>::value_type);
		const auto size = _data.size();

		glBindBuffer(_type, _buffer_id);
#pragma message("SUPPORT MORE THAN JUST STATIC DRAW WHEN POSSIBLE!")
		glBufferData(_type, stride * size, _data.data(), GL_STATIC_DRAW);
	}



	void add_attribute(const GLuint element_count, const GLuint element_type, const GLuint stride, const std::size_t offset)
	{
		std::cout << "attribute id: " << _attribute_id << std::endl;
		glVertexAttribPointer(_attribute_id, element_count, element_type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
		glEnableVertexAttribArray(_attribute_id);
		_attribute_id++;
		
	}

public:
	buffer(const GLuint type, std::vector<T>& data)
		: _type{ type }, _data{ data }
	{
		glGenBuffers(1, &_buffer_id);
		static_bind();
	}
};


class window
{
private:
	GLFWwindow* _window;

public:
	decltype(_window)& handle()
	{
		return _window;
	}

public:
	bool running() const
	{
		return !glfwWindowShouldClose(_window);
	}

public:
	void key_action(int key, auto callable)
	{
		if (glfwGetKey(_window, key) == GLFW_PRESS)
		{
			callable();
		}
	}

public:
	window(const int width, const int height, std::string_view title)
	{
		if (!glfwInit())
		{
			PANIC("Failed to initialize GLFW");
		}

		_window = glfwCreateWindow(width, height, title.data(), NULL, NULL);

		if (_window == NULL)
		{
			PANIC("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(_window);

		if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
		{
			PANIC("Failed to load GLAD function pointers");
		}
	}
};


class camera
{
private:
	glm::vec3 _pos, _dir, _acc;
	glm::vec3 _vel;
	float _yaw, _pitch;

private:
	double _mouse_x, _mouse_y;
	double _mouse_x_old, _mouse_y_old;

private:
	const float _sensitivity;

private:
	GLFWwindow* _window;
	bool _locked;

private:
	static constexpr glm::vec3 _up{ 0, 1, 0 };

public:
	const glm::vec3 forward() const
	{
		return glm::normalize(glm::vec3{ -_dir.x, 0.0f, -_dir.z });
	}

	const glm::vec3 up() const
	{
		return _up;
	}

	const glm::vec3 right() const
	{
		return glm::cross(forward(), up());
	}

private:
	void poll_mouse()
	{
		glfwGetCursorPos(_window, &_mouse_x, &_mouse_y);

		if (_locked)
		{
			_yaw -= static_cast<float>(_mouse_x_old - _mouse_x) * _sensitivity;
			_pitch -= static_cast<float>(_mouse_y_old - _mouse_y) * _sensitivity;

			_yaw = std::fmod(_yaw, glm::two_pi<float>());
			_pitch = glm::clamp(_pitch, glm::radians(-85.0f), glm::radians(85.0f));
		}

		_mouse_x_old = _mouse_x;
		_mouse_y_old = _mouse_y;
	}

	void update_dir()
	{
		_dir =
		{
			glm::cos(_pitch) * glm::cos(_yaw), // x
			glm::sin(_pitch),                  // y
			glm::cos(_pitch) * glm::sin(_yaw), // z
		};
	}

	void integrate(float delta_time)
	{
		_pos += _vel * delta_time;
		_vel += _acc * delta_time;
		_acc = -_vel;

		if (glm::length(_vel) < 0.01f)
		{
			_vel = glm::vec3{ 0.0f };
		}
	}

public:
	decltype(_pos)& pos()
	{
#pragma message("REMOVE THIS FUNCTION WHEN POSSIBLE")
		return _pos;
	}

	decltype(_vel)& vel()
	{
#pragma message("REMOVE THIS FUNCTION WHEN POSSIBLE")
		return _vel;
	}

	decltype(_dir)& dir()
	{
#pragma message("REMOVE THIS FUNCTION WHEN POSSIBLE")
		return _dir;
	}

public:
	void update(float delta_time)
	{
		poll_mouse();
		update_dir();
		integrate(delta_time);

		if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			_locked = true;
		}

		else
		{
			glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			_locked = false;
		}
	}

public:
	camera(const glm::vec3 pos, const float yaw, const float pitch, window& window, const float sensitivity)
		: _pos{ pos }, _yaw{ -yaw }, _pitch{ -pitch }, _window{ window.handle() }, _sensitivity{ sensitivity }
	{
		update_dir();
	}
};

class block
{
public:
	glm::vec3 _color;
	std::uint8_t _shape : 6;
	std::uint8_t _normal;
};

class subchunk
{
public:
	std::array<block*, 16 * 16 * 16> _blocks;

public:
	static constexpr auto CHUNK_LENGTH = 16;
	block*& index(int x, int y, int z)
	{
		return _blocks[x + (CHUNK_LENGTH * (y + (CHUNK_LENGTH * z)))];
	}
};

class chunk
{
public:
	std::array<subchunk, 16> _subchunks;
};

int main(int argc, char** argv)
{
	static constexpr auto WIDTH = 1280, HEIGHT = 720;
	//static constexpr auto WIDTH = 2560, HEIGHT = 1440;
	static constexpr auto CAMERA_SPEED = 5.0f;

	window window{ WIDTH, HEIGHT, "geo" };

	shader_program world_program{ "C:/Users/linkc/Desktop/geo/world" };
	shader_program sky_program{ "C:/Users/linkc/Desktop/geo/sky" };


	chunk* c = new chunk{};

	for (auto s = 0; s < 16; s++)
	{
		for (auto i = 0; i < 16; i++)
		{
			for (auto j = 0; j < 16; j++)
			{
				for (auto k = 0; k < 16; k++)
				{
				}
			}
		}
	}
	
	auto s = new subchunk{};


	for (auto x = 0; x < subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < subchunk::CHUNK_LENGTH; z++)
			{
				s->index(x, y, z) = new block{ glm::vec3{ x / 16.0f, y / 16.0f, z / 16.0f } };
			}
		}
	}



	struct vertex
	{
		glm::vec4 pos;
		glm::vec3 col;
		std::uint8_t normal;
	};

	static std::vector<glm::vec4> verts
	{
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f, -1.0f,  1.0f, 1.0f, },
		{ -1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f,  1.0f, -1.0f, 1.0f, },
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f,  1.0f, -1.0f, 1.0f, },
		{  1.0f, -1.0f,  1.0f, 1.0f, },
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{  1.0f, -1.0f, -1.0f, 1.0f, },
		{  1.0f,  1.0f, -1.0f, 1.0f, },
		{  1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f,  1.0f,  1.0f, 1.0f, },
		{ -1.0f,  1.0f, -1.0f, 1.0f, },
		{  1.0f, -1.0f,  1.0f, 1.0f, },
		{ -1.0f, -1.0f,  1.0f, 1.0f, },
		{ -1.0f, -1.0f, -1.0f, 1.0f, },
		{ -1.0f,  1.0f,  1.0f, 1.0f, },
		{ -1.0f, -1.0f,  1.0f, 1.0f, },
		{  1.0f, -1.0f,  1.0f, 1.0f, },
		{  1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f, -1.0f, -1.0f, 1.0f, },
		{  1.0f,  1.0f, -1.0f, 1.0f, },
		{  1.0f, -1.0f, -1.0f, 1.0f, },
		{  1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f, -1.0f,  1.0f, 1.0f, },
		{  1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f,  1.0f, -1.0f, 1.0f, },
		{ -1.0f,  1.0f, -1.0f, 1.0f, },
		{  1.0f,  1.0f,  1.0f, 1.0f, },
		{ -1.0f,  1.0f, -1.0f, 1.0f, },
		{ -1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f,  1.0f,  1.0f, 1.0f, },
		{ -1.0f,  1.0f,  1.0f, 1.0f, },
		{  1.0f, -1.0f,  1.0f, 1.0f, },
	};

	static std::vector<glm::vec4> cube_vertices
	{
		{  1.0f,  1.0f,  1.0f,    1.0f }, // 0 close top right
		{  1.0f,  1.0f, -1.0f,    1.0f }, // 1 far top right
		{ -1.0f,  1.0f,  1.0f,    1.0f }, // 2 close top left
		{ -1.0f,  1.0f, -1.0f,    1.0f }, // 3 far top left
							      
		{  1.0f, -1.0f,  1.0f,    1.0f }, // 4 close bottom right
		{  1.0f, -1.0f, -1.0f,    1.0f }, // 5 far bottom right
		{ -1.0f, -1.0f,  1.0f,    1.0f }, // 6 close bottom left
		{ -1.0f, -1.0f, -1.0f,    1.0f }, // 7 far bottom left
	};

	static std::vector<GLubyte> close_face{ 0, 2, 6,  6, 4, 0 }, // normal 0
							      top_face{ 3, 2, 0,  0, 1, 3 }, // normal 1
							     left_face{ 3, 7, 6,  6, 2, 3 }, // normal 2
							    right_face{ 0, 4, 5,  5, 1, 0 }, // normal 3
							      far_face{ 1, 5, 7,  7, 3, 1 }, // normal 4
							   bottom_face{ 6, 7, 5,  5, 4, 6 }; // normal 5


	//std::reverse(top_face.begin(), top_face.end());
	//std::reverse(left_face.begin(), left_face.end());
	//std::reverse(right_face.begin(), right_face.end());

	static std::vector<GLubyte> cube_indices(6 * 6);


	//std::reverse(far_face.begin(), far_face.end()); okay
	//std::reverse(close_face.begin(), close_face.end()); okay
	//std::reverse(bottom_face.begin(), bottom_face.end()); okay


	
	std::vector<vertex> world_vertices(cube_vertices.size());
	
	static constexpr auto stride = sizeof(vertex);

	buffer world_vertex_buffer{ GL_ARRAY_BUFFER, world_vertices };
	world_vertex_buffer.add_attribute(4, GL_FLOAT, stride, offsetof(vertex, pos));
	world_vertex_buffer.add_attribute(3, GL_FLOAT, stride, offsetof(vertex, col));
	world_vertex_buffer.add_attribute(1, GL_UNSIGNED_BYTE, stride, offsetof(vertex, normal));
	
	buffer world_index_buffer{ GL_ELEMENT_ARRAY_BUFFER, cube_indices };


	std::vector<glm::vec4> skybox(verts.size());
	buffer sky_vertex_buffer{ GL_ARRAY_BUFFER, skybox };
	sky_vertex_buffer.add_attribute(4, GL_FLOAT, sizeof(glm::vec4), NULL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	//const auto rotation = glm::quarter_pi<float>();
	//const auto t = glm::translate(glm::vec3{ 0.0f, 0.0f, 0.0f });
	//const auto r = glm::rotate(glm::identity<glm::mat4>(), rotation, glm::vec3{ 0.0f, 0.0f, 1.0f });
	//const auto s = glm::scale(glm::vec3{ 5.0, 0.5f, 5.0f });
	//const auto m = t * r * s;
	auto m = glm::mat4(1.0f);


	auto compute_p = [&](float fov)
	{
		return glm::perspectiveFov(glm::radians(fov), static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 1.0f, 10000.0f);
	};

	auto fov = 90.0f;
	auto p = compute_p(fov);

	glfwSwapInterval(0);


	auto last_time = 0.0;

	camera camera{ { 16.0f, 16.0f, 40.0f }, glm::radians(0.0f), glm::radians(0.0f), window, 0.002f};

	while (window.running())
	{
		const auto current_time = glfwGetTime();
		const auto delta_time = static_cast<float>(current_time - last_time);
		last_time = current_time;

		window.key_action(GLFW_KEY_W, [&]() { camera.vel() += camera.forward() * (CAMERA_SPEED * delta_time); });
		window.key_action(GLFW_KEY_S, [&]() { camera.vel() -= camera.forward() * (CAMERA_SPEED * delta_time); });

		window.key_action(GLFW_KEY_D, [&]() { camera.vel() += camera.right() * (CAMERA_SPEED * delta_time); });
		window.key_action(GLFW_KEY_A, [&]() { camera.vel() -= camera.right() * (CAMERA_SPEED * delta_time); });

		window.key_action(GLFW_KEY_SPACE, [&]() { camera.vel() += camera.up() * (CAMERA_SPEED * delta_time); });
		window.key_action(GLFW_KEY_LEFT_SHIFT, [&]() { camera.vel() -= camera.up() * (CAMERA_SPEED * delta_time); });

		if (glfwGetMouseButton(window.handle(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
		{
			fov = 60.0f;
			p = compute_p(fov);
		}

		else
		{
			fov = 90.0f;
			p = compute_p(fov);
		}

		camera.update(delta_time);

		

		const auto v = glm::lookAt(camera.pos(), camera.pos() - camera.dir(), camera.up());

		const auto pv = p * v;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		



		world_program.use();
		world_program.upload_matrix(glm::inverse(pv), "world_imvp");

		for (auto x = 0; x < subchunk::CHUNK_LENGTH; x++)
		{
			for (auto y = 0; y < subchunk::CHUNK_LENGTH; y++)
			{
				for (auto z = 0; z < subchunk::CHUNK_LENGTH; z++)
				{
					for (auto i = 0; i < cube_vertices.size(); i++)
					{
						m = glm::translate(glm::vec3{ x * 2.0f, y * 2.0f, z * 2.0f });
						const auto mvp = pv * m;

						world_vertices[i].pos = mvp * cube_vertices[i];
						world_vertices[i].col = glm::vec3{ (x / 16.0f), y / 16.0f, z / 16.0f };
					}

					cube_indices = {};

					if (y + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x, y + 1, z) == nullptr)
						{
							cube_indices.append_range(top_face);
						}
					}

					else if (y == subchunk::CHUNK_LENGTH - 1)
					{
						cube_indices.append_range(top_face);
					}


					if (y - 1 > 0)
					{
						if (s->index(x, y - 1, z) == nullptr)
						{
							cube_indices.append_range(bottom_face);
						}
					}

					else if (y == 0)
					{
						cube_indices.append_range(bottom_face);
					}


					if (x + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x + 1, y, z) == nullptr)
						{
							cube_indices.append_range(right_face);
						}
					}

					else if (x == subchunk::CHUNK_LENGTH - 1)
					{
						cube_indices.append_range(right_face);
					}


					if (x - 1 > 0)
					{
						if (s->index(x - 1, y, z) == nullptr)
						{
							cube_indices.append_range(left_face);
						}
					}

					else if (x == 0)
					{
						cube_indices.append_range(left_face);
					}


					if (z + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x, y, z + 1) == nullptr)
						{
							cube_indices.append_range(close_face);
						}
					}

					else if (z == subchunk::CHUNK_LENGTH - 1)
					{
						cube_indices.append_range(close_face);
					}


					if (z - 1 > 0)
					{
						if (s->index(x, y, z - 1) == nullptr)
						{
							cube_indices.append_range(far_face);
						}
					}

					else if (z == 0)
					{
						cube_indices.append_range(far_face);
					}

					
					if (cube_indices.size() != 0)
					{
						world_vertex_buffer.static_bind();
						world_index_buffer.static_bind();

						glFrontFace(GL_CCW);
						glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cube_indices.size()), GL_UNSIGNED_BYTE, nullptr);
					}
				}
			}
		}


		const auto sky_m = glm::translate(camera.pos()) * glm::scale(glm::vec3{ 1000.0f });
		const auto sky_mvp = pv * sky_m;
		const auto sky_imvp = glm::inverse(sky_mvp); // inverse for skybox
		for (auto i = 0; i < verts.size(); i++)
		{
			skybox[i] = sky_mvp * verts[i];
		}

		sky_program.use();
		sky_program.upload_matrix(sky_imvp, "sky_imvp");

		sky_vertex_buffer.static_bind();


		glFrontFace(GL_CW);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(skybox.size()));


		glfwSwapBuffers(window.handle());
		glfwPollEvents();
	}
}