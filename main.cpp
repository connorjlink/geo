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

template<typename T>
class buffer
{
private:
	const GLuint _type;
	GLuint _buffer_id;

private:
	const std::vector<T>& _data;

private:
	static GLuint _attribute_id;

public:
	void bind()
	{
		const auto stride = sizeof(std::remove_reference_t<decltype(_data)>::value_type);
		const auto size = _data.size();

		glBindBuffer(_type, _buffer_id);
#pragma message("SUPPORT MORE THAN JUST STATIC DRAW WHEN POSSIBLE!")
		glBufferData(_type, stride * size, _data.data(), GL_STATIC_DRAW);
	}

	void add_attribute(const GLuint element_count, const GLuint stride, const std::size_t offset)
	{
		glVertexAttribPointer(_attribute_id, element_count, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offset));
		glEnableVertexAttribArray(_attribute_id);
		_attribute_id++;
	}

public:
	buffer(const GLuint type, const std::vector<T>& data)
		: _type{ type }, _data{ data }
	{
		glGenBuffers(1, &_buffer_id);
		bind();
	}
};

template<typename T>
GLuint buffer<T>::_attribute_id = 0;


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

int main(int argc, char** argv)
{
	static constexpr auto WIDTH = 1280, HEIGHT = 720;
	//static constexpr auto WIDTH = 2560, HEIGHT = 1440;
	static constexpr auto CAMERA_SPEED = 5.0f;

	window window{ WIDTH, HEIGHT, "geo" };

	shader_program world_program{ "C:/Users/linkc/Desktop/geo/world" };
	shader_program sky_program{ "C:/Users/linkc/Desktop/geo/sky" };


	struct vertex
	{
		glm::vec4 pos;
		glm::vec3 col;
		glm::vec3 norm;
	};

	static std::vector<vertex> verts
	{
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.583f,  0.771f,  0.014f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f,  1.0f, 1.0f, }, { 0.609f,  0.115f,  0.436f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f,  1.0f, 1.0f, }, { 0.327f,  0.483f,  0.844f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f, -1.0f, 1.0f, }, { 0.822f,  0.569f,  0.201f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.435f,  0.602f,  0.223f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f, -1.0f, 1.0f, }, { 0.310f,  0.747f,  0.185f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f,  1.0f, 1.0f, }, { 0.597f,  0.770f,  0.761f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.559f,  0.436f,  0.730f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f, -1.0f, 1.0f, }, { 0.359f,  0.583f,  0.152f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f, -1.0f, 1.0f, }, { 0.483f,  0.596f,  0.789f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f, -1.0f, 1.0f, }, { 0.559f,  0.861f,  0.639f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.195f,  0.548f,  0.859f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.014f,  0.184f,  0.576f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f,  1.0f, 1.0f, }, { 0.771f,  0.328f,  0.970f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f, -1.0f, 1.0f, }, { 0.406f,  0.615f,  0.116f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f,  1.0f, 1.0f, }, { 0.676f,  0.977f,  0.133f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f,  1.0f, 1.0f, }, { 0.971f,  0.572f,  0.833f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f, -1.0f, 1.0f, }, { 0.140f,  0.616f,  0.489f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f,  1.0f, 1.0f, }, { 0.997f,  0.513f,  0.064f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f, -1.0f,  1.0f, 1.0f, }, { 0.945f,  0.719f,  0.592f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f,  1.0f, 1.0f, }, { 0.543f,  0.021f,  0.978f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f,  1.0f, 1.0f, }, { 0.279f,  0.317f,  0.505f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f, -1.0f, 1.0f, }, { 0.167f,  0.620f,  0.077f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f, -1.0f, 1.0f, }, { 0.347f,  0.857f,  0.137f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f, -1.0f, 1.0f, }, { 0.055f,  0.953f,  0.042f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f,  1.0f, 1.0f, }, { 0.714f,  0.505f,  0.345f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f,  1.0f, 1.0f, }, { 0.783f,  0.290f,  0.734f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f,  1.0f, 1.0f, }, { 0.722f,  0.645f,  0.174f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f, -1.0f, 1.0f, }, { 0.302f,  0.455f,  0.848f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f, -1.0f, 1.0f, }, { 0.225f,  0.587f,  0.040f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f,  1.0f, 1.0f, }, { 0.517f,  0.713f,  0.338f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f, -1.0f, 1.0f, }, { 0.053f,  0.959f,  0.120f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f,  1.0f, 1.0f, }, { 0.393f,  0.621f,  0.362f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f,  1.0f,  1.0f, 1.0f, }, { 0.673f,  0.211f,  0.457f, }, { 0.0f, 0.0f, 0.0f }, },
		{ { -1.0f,  1.0f,  1.0f, 1.0f, }, { 0.820f,  0.883f,  0.371f, }, { 0.0f, 0.0f, 0.0f }, },
		{ {  1.0f, -1.0f,  1.0f, 1.0f, }, { 0.982f,  0.099f,  0.879f, }, { 0.0f, 0.0f, 0.0f }, },
	};

	std::vector<vertex> skybox_verts(verts.size());
	std::copy(verts.begin(), verts.end(), skybox_verts.begin());

	std::vector<vertex> skybox(skybox_verts.size());

	//verts.reserve(verts.size() * 16);
	//
	//verts.insert(verts.end(), verts.begin(), verts.end());
	//verts.insert(verts.end(), verts.begin(), verts.end());
	//verts.insert(verts.end(), verts.begin(), verts.end());
	//verts.insert(verts.end(), verts.begin(), verts.end());

	std::vector<vertex> world(verts.size());
	
	static constexpr auto stride = sizeof(vertex);


	buffer world_vertex_buffer{ GL_ARRAY_BUFFER, world };
	world_vertex_buffer.add_attribute(4, stride, offsetof(vertex, pos));
	world_vertex_buffer.add_attribute(3, stride, offsetof(vertex, col));
	world_vertex_buffer.add_attribute(3, stride, offsetof(vertex, norm));


	buffer sky_vertex_buffer{ GL_ARRAY_BUFFER, skybox };
	sky_vertex_buffer.add_attribute(4, stride, offsetof(vertex, pos));

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//const auto rotation = glm::quarter_pi<float>();
	//const auto t = glm::translate(glm::vec3{ 0.0f, 0.0f, 0.0f });
	//const auto r = glm::rotate(glm::identity<glm::mat4>(), rotation, glm::vec3{ 0.0f, 0.0f, 1.0f });
	//const auto s = glm::scale(glm::vec3{ 5.0, 0.5f, 5.0f });
	//const auto m = t * r * s;
	auto m = glm::mat4(1.0f);

	//view matrix is created inside the loop

	auto fov = 90.0f;

	//compute per-vertex normals
	for (auto i = 0; i < verts.size(); i += 3)
	{
		glm::vec3 v0 = m * verts[i + 0].pos;
		glm::vec3 v1 = m * verts[i + 1].pos;
		glm::vec3 v2 = m * verts[i + 2].pos;

		glm::vec3 d1 = v1 - v0;
		glm::vec3 d2 = v2 - v0;

		glm::vec3 norm = glm::normalize(glm::cross(d1, d2));

		verts[i + 0].norm = norm;
		verts[i + 1].norm = norm;
		verts[i + 2].norm = norm;
	}


	for (auto i = 0; i < verts.size(); i++)
	{
		world[i].col = verts[i].col;
		world[i].norm = verts[i].norm;
	}

	glfwSwapInterval(0);


	auto last_time = 0.0;

	camera camera{ { 0.0f, 2.0f, 6.0f }, glm::radians(-90.0f), glm::radians(-10.0f), window, 0.002f};

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
		}

		else
		{
			fov = 90.0f;
		}

		camera.update(delta_time);

		const auto p = glm::perspectiveFov(glm::radians(fov), static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.1f, 100.0f);

		const auto v = glm::lookAt(camera.pos(), camera.pos() - camera.dir(), camera.up());

		const auto pv = p * v;

		const auto sky_m = glm::translate(camera.pos()) * glm::scale(glm::vec3{ 50.0f });
		const auto sky_mvp = pv * sky_m;
		const auto sky_imvp = glm::inverse(sky_mvp); // inverse for skybox

		for (auto i = 0; i < skybox_verts.size(); i++)
		{
			skybox[i].pos = sky_mvp * skybox_verts[i].pos;
		}

		glClear(GL_DEPTH_BUFFER_BIT);

		auto len = 16;

		for (auto k = 0; k < len * len * len; k++)
		{
			for (auto i = 0; i < verts.size(); i++)
			{
				auto x = (k / (len * len));
				auto y = ((k / len) % len);
				auto z = (k % len);

				m = glm::translate(glm::vec3{ x * 2.0f, y * 2.0f, z * 2.0f });

				const auto mvp = pv * m;

				world[i].pos = mvp * verts[i].pos;
			}

			world_program.use();

			world_vertex_buffer.bind();
			glFrontFace(GL_CCW);
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));
		}

		sky_program.use();
		sky_program.upload_matrix(sky_imvp, "sky_imvp");

		sky_vertex_buffer.bind();
		glFrontFace(GL_CW);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(skybox_verts.size()));

		glfwSwapBuffers(window.handle());
		glfwPollEvents();
	}
}