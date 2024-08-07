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
	void bind(const GLuint hint = GL_STATIC_DRAW)
	{
		const auto stride = sizeof(std::remove_reference_t<decltype(_data)>::value_type);
		const auto size = _data.size();

		glBindBuffer(_type, _buffer_id);
#pragma message("SUPPORT MORE THAN JUST STATIC DRAW WHEN POSSIBLE!")
		glBufferData(_type, stride * size, _data.data(), hint);
	}

	void add_attribute(const GLuint element_count, const GLuint element_type, const GLuint stride, const std::size_t offset)
	{
		std::cout << "attribute id: " << _attribute_id << std::endl;
		glVertexAttribPointer(_attribute_id, element_count, element_type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
		glEnableVertexAttribArray(_attribute_id);
		_attribute_id++;
	}

	void base()
	{
		glBindBufferBase(_type, _attribute_id, _buffer_id);
	}

public:
	buffer(const GLuint type, std::vector<T>& data)
		: _type{ type }, _data{ data }
	{
		glGenBuffers(1, &_buffer_id);
		bind();
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
		_pos += 4.0f * _vel * delta_time;
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

struct vertex
{
	glm::vec4 pos;
	glm::vec3 col;
};

class block
{
public:
	glm::vec3 _color;
	std::vector<vertex> _vertices;
	std::vector<GLuint> _indices;
	std::vector<GLuint> _normals;

public:
	block(const glm::vec3 color)
		: _color{ color }
	{
		_vertices = {};
		_indices = {};
		_normals = {};
	}
};

class subchunk
{
public:
	static constexpr auto CHUNK_LENGTH = 32;

public:
	std::array<block*, CHUNK_LENGTH * CHUNK_LENGTH * CHUNK_LENGTH> _blocks;

public:
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
					// TODO: chunk rendering here
				}
			}
		}
	}
	
	



	

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

	for (auto i = 0; i < verts.size(); i += 3)
	{
		std::swap(verts[i], verts[i + 2]);
	}

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

	static std::vector<GLuint> close_face{ 0, 2, 6,  6, 4, 0 }, // normal 0
							     top_face{ 3, 2, 0,  0, 1, 3 }, // normal 1
							    left_face{ 3, 7, 6,  6, 2, 3 }, // normal 2
							   right_face{ 0, 4, 5,  5, 1, 0 }, // normal 3
							     far_face{ 1, 5, 7,  7, 3, 1 }, // normal 4
							  bottom_face{ 6, 7, 5,  5, 4, 6 }; // normal 5

	enum
	{
		CLOSE_FACE = 0,
		TOP_FACE,
		LEFT_FACE,
		RIGHT_FACE,
		FAR_FACE,
		BOTTOM_FACE,
	};

	auto s = new subchunk{};

	for (auto x = 0; x < subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < subchunk::CHUNK_LENGTH; z++)
			{
				auto xyz = glm::vec3{ x, y, z };

				auto center = glm::vec3{ subchunk::CHUNK_LENGTH / 2 };

				if (glm::distance(xyz, center) < subchunk::CHUNK_LENGTH / 2)
				{
					s->index(x, y, z) = new block{ glm::vec3
					{ 
						x / static_cast<float>(subchunk::CHUNK_LENGTH),
						y / static_cast<float>(subchunk::CHUNK_LENGTH), 
						z / static_cast<float>(subchunk::CHUNK_LENGTH) 
					} };
				}
			}
		}
	}

	auto stride_accumulator = 0;

	for (auto x = 0; x < subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < subchunk::CHUNK_LENGTH; z++)
			{
				auto b = s->index(x, y, z);

				if (b != nullptr)
				{
					for (auto i = 0; i < cube_vertices.size(); i++)
					{
						const auto m = glm::translate(glm::vec3{ x * 2.0f, y * 2.0f, z * 2.0f });

						vertex v{};

						v.pos = m * cube_vertices[i];

						v.col = glm::vec3
						{
							(x + ((cube_vertices[i].x + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
							(y + ((cube_vertices[i].y + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
							(z + ((cube_vertices[i].z + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
						};

						b->_vertices.emplace_back(v);
					}

					if (y + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x, y + 1, z) == nullptr)
						{
							b->_indices.append_range(top_face);
							b->_normals.emplace_back(TOP_FACE);
						}
					}

					else if (y == subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(top_face);
						b->_normals.emplace_back(TOP_FACE);
					}


					if (y - 1 > 0)
					{
						if (s->index(x, y - 1, z) == nullptr)
						{
							b->_indices.append_range(bottom_face);
							b->_normals.emplace_back(BOTTOM_FACE);
						}
					}

					else if (y == 0)
					{
						b->_indices.append_range(bottom_face);
						b->_normals.emplace_back(BOTTOM_FACE);
					}


					if (x + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x + 1, y, z) == nullptr)
						{
							b->_indices.append_range(right_face);
							b->_normals.emplace_back(RIGHT_FACE);
						}
					}

					else if (x == subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(right_face);
						b->_normals.emplace_back(RIGHT_FACE);
					}


					if (x - 1 > 0)
					{
						if (s->index(x - 1, y, z) == nullptr)
						{
							b->_indices.append_range(left_face);
							b->_normals.emplace_back(LEFT_FACE);
						}
					}

					else if (x == 0)
					{
						b->_indices.append_range(left_face);
						b->_normals.emplace_back(LEFT_FACE);
					}


					if (z + 1 < subchunk::CHUNK_LENGTH)
					{
						if (s->index(x, y, z + 1) == nullptr)
						{
							b->_indices.append_range(close_face);
							b->_normals.emplace_back(CLOSE_FACE);
						}
					}

					else if (z == subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(close_face);
						b->_normals.emplace_back(CLOSE_FACE);
					}


					if (z - 1 > 0)
					{
						if (s->index(x, y, z - 1) == nullptr)
						{
							b->_indices.append_range(far_face);
							b->_normals.emplace_back(FAR_FACE);
						}
					}

					else if (z == 0)
					{
						b->_indices.append_range(far_face);
						b->_normals.emplace_back(FAR_FACE);
					}


					for (auto& i : b->_indices)
					{
						i += stride_accumulator;
					}

					stride_accumulator += b->_vertices.size();
				}
			}
		}
	}

	std::vector<vertex> world_vertices;
	std::vector<GLuint> world_indices;
	std::vector<GLuint> world_normals;

	for (auto x = 0; x < subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < subchunk::CHUNK_LENGTH; z++)
			{
				auto b = s->index(x, y, z);

				if (b != nullptr)
				{
					for (auto& v : b->_vertices)
					{
						world_vertices.emplace_back(v);
					}

					for (auto& i : b->_indices)
					{
						world_indices.emplace_back(i);
					}

					for (auto& n : b->_normals)
					{
						world_normals.emplace_back(n);
					}
				}
			}
		}

	}

	std::vector<vertex> world_vertices_transform = world_vertices;
	

	//std::vector<GLuint> world_normals(6);

	
	static constexpr auto stride = sizeof(vertex);

	buffer world_vertex_buffer{ GL_ARRAY_BUFFER, world_vertices_transform };
	world_vertex_buffer.add_attribute(4, GL_FLOAT, stride, offsetof(vertex, pos));
	world_vertex_buffer.add_attribute(3, GL_FLOAT, stride, offsetof(vertex, col));
	
	buffer world_normal_buffer{ GL_SHADER_STORAGE_BUFFER, world_normals };
	//world_normal_buffer.bind();
	world_normal_buffer.base();
	
	//GLuint normal_buffer;
	//glGenBuffers(1, &normal_buffer);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, normal_buffer);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(decltype(world_normals)::value_type)* world_normals.size(), world_normals.data(), GL_DYNAMIC_DRAW);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, normal_buffer);

	buffer world_index_buffer{ GL_ELEMENT_ARRAY_BUFFER, world_indices };


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
	auto m = glm::mat4(1.0f), sky_m = glm::mat4(1.0f), sky_mvp = glm::mat4(1.0f), sky_imvp = glm::mat4(1.0f);


	auto compute_p = [&](float fov)
	{
		return glm::perspectiveFov(glm::radians(fov), static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 1.0f, 10000.0f);
	};

	auto fov = 90.0f;
	auto p = compute_p(fov);

	glfwSwapInterval(0);


	auto last_time = 0.0;
	auto last_update = 0.0;

	camera camera{ { 16.0f, 16.0f, 40.0f }, glm::radians(0.0f), glm::radians(0.0f), window, 0.002f};

	// let's make sure our timer stuff fires initially
	auto timer = 1.1; 

	
	world_index_buffer.bind();
	world_normal_buffer.bind();

	while (window.running())
	{
		const auto current_time = glfwGetTime();
		const auto delta_time = static_cast<float>(current_time - last_time);
		last_time = current_time;

		if (timer > 1.0)
		{
   			glfwSetWindowTitle(window.handle(), std::format("geo - {} FPS", static_cast<int>(1.0 / delta_time)).c_str());
			last_update = current_time;
		}


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

		glClear(GL_DEPTH_BUFFER_BIT);


		world_program.use();
		world_vertex_buffer.bind(GL_DYNAMIC_DRAW);

		for (auto i = 0; i < world_vertices.size(); i++)
		{
			world_vertices_transform[i].pos = pv * world_vertices[i].pos;
		}

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(world_indices.size()), GL_UNSIGNED_INT, nullptr);


		sky_m = glm::translate(camera.pos()) * glm::scale(glm::vec3{ 1000.0f });
		sky_mvp = pv * sky_m;
		sky_imvp = glm::inverse(sky_mvp);

		if (timer > 1.0)
		{
			for (auto i = 0; i < verts.size(); i++)
			{
				skybox[i] = sky_mvp * verts[i];
			}
		}

		sky_program.use();
		sky_program.upload_matrix(sky_imvp, "sky_imvp");

		sky_vertex_buffer.bind();

		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(skybox.size()));

		glfwSwapBuffers(window.handle());
		glfwPollEvents();

		// TODO: resolve the timing stuff so this can go at the top of the loop with the rest of it
		timer = current_time - last_update;
	}
}