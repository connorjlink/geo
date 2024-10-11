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
#include <numeric>

#include <array>
#include <cstdlib>
#include <cmath>


#include <windows.h>
#include <winerror.h>
#include <comdef.h>

//#include <gl/gl.h>
#include "glad.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")

typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext,
	const int* attribList);
wglCreateContextAttribsARB_type* wglCreateContextAttribsARB;

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt for all values
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001

typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int* piAttribIList,
	const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
wglChoosePixelFormatARB_type* wglChoosePixelFormatARB;

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt for all values
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023

#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

#define PANIC(x) std::println(std::cerr, x); std::cin.get(); std::exit(EXIT_FAILURE)

#define panic(x) std::print("{} @ {}", x, __FUNCTION__)

namespace geo
{
	using platform_type = float;

	template<typename T>
	consteval platform_type native(T param)
	{
		return static_cast<platform_type>(param);
	}


#pragma region Floating Point Manipulation
	template<typename T = platform_type>
	consteval T pi()
	{
		return T{ 3.14159265358979323846l };
	}

	template<typename T = platform_type>
	consteval T two_pi()
	{
		return 2 * pi();
	}

	template<typename T = platform_type>
	constexpr T radians(T degrees)
	{
		constexpr auto factor = pi() / T{ 180 };
		return degrees * factor;
	}

	template<typename T = platform_type>
	constexpr T degrees(T radians)
	{
		constexpr auto factor = T{ 180 } / pi();
		return radians * factor;
	}
#pragma endregion


	template<std::size_t M = 4, std::size_t N = 4, typename T = platform_type>
	class mat
	{
	private:
		std::array<std::array<T, N>, M> _data;

	public:
		constexpr auto operator[](std::size_t i, std::size_t j)
		{
			return _data[i][j];
		}
	};

	template<typename T>
	using basic_mat2 = mat<2, 2, T>;
	using mat2 = basic_mat2<platform_type>;

	template<typename T>
	using basic_mat3 = mat<3, 3, T>;
	using mat3 = basic_mat3<platform_type>;

	template<typename T>
	using basic_mat4 = mat<4, 4, T>;
	using mat4 = basic_mat4<platform_type>;

#pragma region Matrix Manipulation
	template<std::size_t M = 4, typename T = platform_type>
	constexpr mat<M, M, T> identity()
	{
		mat<M, M, T> out{};

		for (auto ij = 0; ij < M; ij++)
		{
			out[ij, ij] = T{ 1 };
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr mat<M, M, T> null()
	{
		return {};
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T determinant(const mat<M, M, T>& matrix)
	{
#pragma message("TODO: verify that this function works as expected")
		auto sum = T{ 0 };

		//std::iota()
		std::array<T, M> is;
		std::iota(is.begin(), is.end(), 0);

		std::array<T, M> js;
		std::iota(js.begin(), js.end(), 1);

		auto diagonal_product = [&]()
		{
			auto product = T{ 1 };

			// for each element each diagonal
			for (auto l = 0; l < M; l++)
			{
				const auto i = is[l];
				const auto j = js[l];

				product *= matrix[i, j];
			}

			return product;
		};

		auto update_js = [&]()
		{
			for (auto k = 0; k < M; k++)
			{
				// increment and clamp the range so indices wrap back around
				js[k]++;
				js[k] %= M;
			}
		};

		// for each forward diagonal
		for (auto k = 0; k < M; k++)
		{
			sum += diagonal_product();
			update_js();
		}

		// for each backward diagonal
		for (auto k = 0; k < M; k++)
		{
			sum -= diagonal_product();
			update_js();
		}

		return sum;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr mat<M, M, T> translation(const vec<M - 1, T>& vector)
	{
		auto out = identity<M, T>();

		for (auto i = 0; i < M - 1; i++)
		{
			out[i, M - 1] = vector[i];
		}

		return out;
	}

	template<typename T = platform_type>
	constexpr mat<4, 4, T> perspective(T fov, T width, T height, T front, T back)
	{
		// NOTE: fov is measured in radians
		// NOTE: front -> near, back -> far

		const auto reciprocal_aspect = height / width;
		const auto reciprocal_tan = 1 / std::tan(fov / 2);
		const auto difference = back - front;

		mat<4, 4, T> out{};

		out[0, 0] = reciprocal_tan * reciprocal_aspect;
		out[1, 1] = reciprocal_tan;
		out[2, 2] = -(back + front) / difference;
		out[2, 3] = -(2 * back * front) / difference;
		out[3, 2] = -1;

		return out;
	}

	template<std::size_t M = 4, std::size_t N = 4, typename T = platform_type>
	constexpr mat<M, N, T> scale(const mat<M, N, T>& matrix, T scalar)
	{
		mat<M, N, T> out{};

		for (auto i = 0; i < M; i++)
		{
			for (auto j = 0; j < N; j++)
			{
				out[i, j] = matrix[i, j] * scalar;
			}
		}

		return out;
	}

	template<std::size_t M = 4, std::size_t R = 4, std::size_t N = 4, typename T = platform_type>
	constexpr mat<M, N, T> multiply(const mat<M, R, T>& matrix1, const mat<R, N, T>& matrix2)
	{
		mat<M, N, T> out{};

		for (auto i = 0; i < M; i++)
		{
			for (auto j = 0; j < N; j++)
			{
				for (auto k = 0; k < M; k++)
				{
					out[i, j] += matrix1[i, k] * matrix2[k, j];
				}
			}
		}

		return out;
	}

	template<std::size_t M = 4, std::size_t N = 4, typename T = platform_type>
	constexpr vec<N, T> apply(const mat<M, N, T>& matrix, const vec<N, T>& vector)
	{
		vec<N, T> out{};

		for (auto i = 0; i < M; i++)
		{
			for (auto j = 0; j < N; j++)
			{
				out[i] += matrix[i, j] * vector[j];
			}
		}

		return out;
	}
#pragma endregion
	

	template<std::size_t M = 4, typename T = platform_type>
	class vec
	{
	private:
		std::array<M, T> _data;

	public:
		auto& operator[](std::size_t i)
		{
			return _data[i];
		}
	};

	template<typename T>
	using basic_vec2 = vec<2, T>;
	using vec2 = basic_vec2<platform_type>;

	template<typename T>
	using basic_vec3 = vec<3, T>;
	using vec3 = basic_vec3<platform_type>;

	template<typename T>
	using basic_vec4 = vec<4, T>;
	using vec4 = basic_vec4<platform_type>;


#pragma region Vector Manipulation
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> broadcast(T scalar)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = scalar;
		}

		return out;
	}

	template<std::size_t M2 = 3, std::size_t M1, typename T = platform_type>
		requires (M2 < M1)
	constexpr vec<M2, T> truncate(const vec<M1, T>& vector)
	{
		vec<M2, T> out{};

		for (auto i = 0; i < M2; i++)
		{
			out[i] = vector[i];
		}

		return out;
	}

	template<std::size_t M2 = 4, std::size_t M1, typename T = platform_type> 
		requires (M2 > M1)
	constexpr vec<M2, T> extend(const vec<M1, T>& vector, T scalar = T{ 1 })
	{
		vec<M2, T> out{};

		for (auto i = 0; i < M1; i++)
		{
			out[i] = vector[i];
		}

		for (auto i = M1; i < M2; i++)
		{
			out[i] = scalar;
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> scale(const vec<M, T>& vector, T scalar)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = vector[i] * scalar;
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> invert(const vec<M, T>& vector)
	{
		const auto result = scale(vector, T{ -1 });
		return result;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> multiply(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = vector1[i] * vector2[i];
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> accumulate(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = vector1[i] + vector2[i];
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T total(const vec<M, T>& vector)
	{
		auto out = T{ 0 };

		for (auto i = 0; i < M; i++)
		{
			out += vector[i];
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T magnitude(const vec<M, T>& vector)
	{
		const auto product = multiply(vector, vector);
		const auto sum = total(p);
		const auto root = std::sqrt(s);

		return root;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T>&& normalize(const vec<M, T>& vector)
	{
		const auto length = magnitude(vector);
		const auto factor = 1 / length;
		const auto result = scale(vector, factor);

		return result;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T distance(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		const auto reverse = invert(vector2);
		const auto difference = accumulate(vector1, reverse);
		const auto length = magnitude(difference);

		return length;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T dot(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		const auto product = multiply(vector1, vector2);
		const auto sum = total(product);

		return sum;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> cross(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		vec<M, T> out{};

		return out;
	}
#pragma endregion

}
class Shader
{
private:
	const GLuint _type;
	const GLuint _program_id;
	GLuint _shader_id;

public:
	Shader(const GLuint type, const GLuint program_id, std::string_view filepath)
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

class shader_factory
{
private:
	const GLuint _program_id;
	std::vector<Shader> _shaders;

public:
	shader_factory(const GLuint program_id)
		: _program_id{ program_id }, _shaders{}
	{
	}

public:
	void compile_shader(const GLuint type, std::string_view filepath)
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

class shader_program
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

	void upload_matrix(const geo::mat4& matrix, const std::string& identifier)
	{
		auto matrix_id = locate_uniform(identifier);
		glUniformMatrix4fv(matrix_id, 1, GL_FALSE, geo::value_ptr(matrix));
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
	HWND _hwnd;
	HDC _hdc;
	HGLRC _hrc;

public:
	decltype(_hwnd)& hwnd()
	{
		return _hwnd;
	}

public:
	static bool key_pressed(int key)
	{
		// key is down
		static constexpr auto PRESSED = 0x8000;

		// key was pressed since last call to GetAsyncKeyState()
		static constexpr auto PRESSED_NEW = 0x1;

		return (GetAsyncKeyState(key) & PRESSED);
	}

public:
	void key_action(int key, auto callable)
	{
		if (key_pressed(key))
		{
			callable();
		}
	}

public:
	void swap()
	{
		SwapBuffers(_hdc);
	}

private:
	static HGLRC OpenGLBindContext(HDC hdc)
	{
		PIXELFORMATDESCRIPTOR pfd{};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 32;
		pfd.cAlphaBits = 8;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, pixelFormat, &pfd);

		HGLRC context = wglCreateContext(hdc);
		wglMakeCurrent(hdc, context);
		return context;
	}


private:
	static void* GetAnyGLFuncAddress(const char* name)
	{
		void* p = (void*)wglGetProcAddress(name); // load newer functions via wglGetProcAddress
		if (p == 0 ||
			(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
			(p == (void*)-1)) // does it return NULL - i.e. is the function not found?
		{
			// could be an OpenGL 1.1 function
			HMODULE module = LoadLibraryA("opengl32.dll");
			p = (void*)GetProcAddress(module, name); // then import directly from GL lib
		}

		return p;
	}

public:
	window(const int width, const int height, std::wstring_view title, WNDPROC proc)
	{
		WNDCLASSEX wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.cbClsExtra = NULL;
		wcex.cbWndExtra = NULL;
		wcex.hInstance = GetModuleHandle(NULL);
		// NOTE: to capture double-click events, specify CS_DBLCLKS
		wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = proc;
		wcex.lpszClassName = title.data();
		wcex.hCursor = LoadCursor(wcex.hInstance, IDC_ARROW);
		
		const auto icon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
		wcex.hIcon = icon;
		wcex.hIconSm = icon;

		wcex.hbrBackground = NULL;

		const auto window_class = RegisterClassEx(&wcex);
		if (!window_class)
		{
			PANIC("Failed to register Win32 window class");
		}
		

		constexpr auto style = ((WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_THICKFRAME) & ~WS_MAXIMIZEBOX;

		_hwnd = CreateWindowEx(NULL, title.data(), title.data(), style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wcex.hInstance, NULL);
		if (!_hwnd)
		{
			PANIC("Failed to create Win32 window");
		}

		_hdc = GetDC(_hwnd);
		_hrc = OpenGLBindContext(_hdc);

		if (!gladLoadGL())
		{
			PANIC("Failed to initialize GLAD");
		}

		if (!gladLoadGLLoader((GLADloadproc)GetAnyGLFuncAddress)) {
			_com_error err{ (HRESULT)GetLastError() };
			auto text = err.ErrorMessage();
			MessageBox(nullptr, text, L"Error", MB_OK);
			return;
		}

		// Load the wglSwapIntervalEXT function
		typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALEXTPROC)(int interval);
		PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetAnyGLFuncAddress("wglSwapIntervalEXT");
		if (wglSwapIntervalEXT == nullptr) {
			MessageBox(nullptr, L"Failed to load wglSwapIntervalEXT", L"Error", MB_OK);
		}
		wglSwapIntervalEXT(0);
		//const auto pointers = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(wglGetProcAddress));
		//if (!pointers)
		//{
		//	_com_error err(GetLastError());
		//	auto errorMsg = err.ErrorMessage();
		//	MessageBoxW(_hwnd, errorMsg, L"Failure", MB_OK);
		//	PANIC("Failed to load GLAD function pointers");
		//}
	}
};


class Camera
{
private:
	geo::vec3 _pos, _dir, _acc;
	geo::vec3 _vel;
	float _yaw, _pitch;

private:
	POINT _mouse;
	POINT _mouse_old;

private:
	const float _sensitivity;

private:
	bool _locked;

private:
	static constexpr geo::vec3 _up{ 0.0f, 1.0f, 0.0f };

public:
	const geo::vec3 forward() const
	{
		const auto result = geo::normalize(geo::vec3{ -_dir[0], 0.0f, -_dir.z });
		return result;
	}

	const geo::vec3 up() const
	{
		return _up;
	}

	const geo::vec3 right() const
	{
		return geo::cross(forward(), up());
	}

private:
	void poll_mouse()
	{
		GetCursorPos(&_mouse);

		if (_locked)
		{
			_yaw -= (_mouse_old.x - _mouse.x) * _sensitivity;
			_pitch -= (_mouse_old.y - _mouse.y) * _sensitivity;

			_yaw = std::fmod(_yaw, geo::two_pi());
			_pitch = std::clamp(_pitch, geo::radians(-85.0f), geo::radians(85.0f));
		}

		_mouse_old = _mouse;
	}

	void update_dir()
	{
		_dir =
		{
			std::cos(_pitch) * std::cos(_yaw), // x
			std::sin(_pitch),                  // y
			std::cos(_pitch) * std::sin(_yaw), // z
		};
	}

	void integrate(float delta_time)
	{
		_pos = geo::accumulate(_pos, geo::scale(_vel, delta_time * 4.0f));
		_vel = geo::accumulate(_vel, geo::scale(_acc, delta_time));
		_acc = geo::invert(_vel);

		if (geo::magnitude(_vel) < 0.01f)
		{
			_vel = geo::broadcast<3>(0.0f);
		}
	}

	void try_lock()
	{
		if (window::key_pressed(VK_RBUTTON))
		{
			//ShowCursor(FALSE);
			_locked = true;
		}

		else
		{
			///ShowCursor(TRUE);
			_locked = false;
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
		try_lock();
	}

public:
	Camera(const geo::vec3 pos, const float yaw, const float pitch, const float sensitivity)
		: _pos{ pos }, _yaw{ -yaw }, _pitch{ -pitch }, _sensitivity{ sensitivity }
	{
		_acc = geo::broadcast<3>(0.0f);
		_vel = geo::broadcast<3>(0.0f);
		update_dir();
	}
};

struct vertex
{
	geo::vec4 pos;
	geo::vec3 col;
};

class Block
{
public:
	geo::vec3 _color;
	std::vector<vertex> _vertices;
	std::vector<GLuint> _indices;
	std::vector<GLuint> _normals;

public:
	Block(const geo::vec3 color)
		: _color{ color }
	{
		_vertices = {};
		_indices = {};
		_normals = {};
	}
};

class Subchunk
{
public:
	static constexpr auto CHUNK_LENGTH = 32;

private:
	std::array<std::array<std::array<Block*, CHUNK_LENGTH>, CHUNK_LENGTH>, CHUNK_LENGTH> _blocks;

public:
	Block* operator[](std::size_t x, std::size_t y, std::size_t z)
	{
		return _blocks[x][y][z];
	}

public:
	Subchunk()
	{
		_blocks = {};
	}
};

class Chunk
{
public:
	static constexpr auto CHUNK_HEIGHT = 16;

private:
	std::array<Subchunk, CHUNK_HEIGHT> _subchunks;

public:
	Subchunk& operator[](size_t x)
	{
		return _subchunks[x];
	}

public:
	Chunk()
	{
		_subchunks = {};
	}
};

static constexpr auto WIDTH = 1280, HEIGHT = 720;
//static constexpr auto WIDTH = 2560, HEIGHT = 1440;
static constexpr auto CAMERA_SPEED = 5.0f;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

window _window{ WIDTH, HEIGHT, L"geo", &WndProc };

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

int APIENTRY wWinMain(_In_     HINSTANCE hinstance,
					  _In_opt_ HINSTANCE hprevinstance,
					  _In_     LPWSTR    lpcmdline,
					  _In_     INT       ncmdshow)
{
	UNREFERENCED_PARAMETER(hprevinstance);
	UNREFERENCED_PARAMETER(lpcmdline);

	

	ShowWindow(_window.hwnd(), ncmdshow);
	UpdateWindow(_window.hwnd());


	shader_program world_program{ "C:/Users/linkc/Desktop/geo/world" };
	shader_program sky_program{ "C:/Users/linkc/Desktop/geo/sky" };


	Chunk* c = new Chunk{};

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
	
	



	

	static std::vector<geo::vec4> verts
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

	static std::vector<geo::vec4> cube_vertices
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

	auto subchunk = Subchunk{};

	constexpr auto whole = geo::native(Subchunk::CHUNK_LENGTH);
	constexpr auto half = whole / 2;

	for (auto x = 0; x < Subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < Subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < Subchunk::CHUNK_LENGTH; z++)
			{
				const auto xyz = geo::vec3{ x, y, z };
				const auto center = geo::broadcast<3>(half);

				const auto distance = geo::distance(xyz, center);

				if (distance < half)
				{
					const auto result = geo::scale(xyz, whole);
					subchunk[x, y, z] = new Block{ result };
				}
			}
		}
	}

	auto stride_accumulator = 0;

	for (auto x = 0; x < Subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < Subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < Subchunk::CHUNK_LENGTH; z++)
			{
				auto b = subchunk[x, y, z];

				const auto xyz = geo::vec3{ x, y, z };
				const auto doubled = geo::scale(xyz, 2.0f);

				if (b != nullptr)
				{
					for (auto i = 0; i < cube_vertices.size(); i++)
					{
						const auto m = geo::translation(doubled);

						vertex v{};

						v.pos = geo::apply(m, cube_vertices[i]);

						const auto sum = geo::accumulate(cube_vertices[i], geo::broadcast(1.0f));
						const auto quotient = geo::truncate<3>(geo::scale(sum, 1 / 2.0f));
						const auto total = geo::accumulate(xyz, quotient);
						
						v.col = geo::scale(total, 1 / whole);

						/*v.col = geo::vec3
						{
							(x + ((cube_vertices[i].x + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
							(y + ((cube_vertices[i].y + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
							(z + ((cube_vertices[i].z + 1.0f) / 2.0f)) / static_cast<float>(subchunk::CHUNK_LENGTH),
						};*/

						b->_vertices.emplace_back(v);
					}

					if (y + 1 < Subchunk::CHUNK_LENGTH)
					{
						if (subchunk[x, y + 1, z] == nullptr)
						{
							b->_indices.append_range(top_face);
							b->_normals.emplace_back(TOP_FACE);
						}
					}

					else if (y == Subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(top_face);
						b->_normals.emplace_back(TOP_FACE);
					}


					if (y - 1 > 0)
					{
						if (subchunk[x, y - 1, z] == nullptr)
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


					if (x + 1 < Subchunk::CHUNK_LENGTH)
					{
						if (subchunk[x + 1, y, z] == nullptr)
						{
							b->_indices.append_range(right_face);
							b->_normals.emplace_back(RIGHT_FACE);
						}
					}

					else if (x == Subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(right_face);
						b->_normals.emplace_back(RIGHT_FACE);
					}


					if (x - 1 > 0)
					{
						if (subchunk[x - 1, y, z] == nullptr)
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


					if (z + 1 < Subchunk::CHUNK_LENGTH)
					{
						if (subchunk[x, y, z + 1] == nullptr)
						{
							b->_indices.append_range(close_face);
							b->_normals.emplace_back(CLOSE_FACE);
						}
					}

					else if (z == Subchunk::CHUNK_LENGTH - 1)
					{
						b->_indices.append_range(close_face);
						b->_normals.emplace_back(CLOSE_FACE);
					}


					if (z - 1 > 0)
					{
						if (subchunk[x, y, z - 1] == nullptr)
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

	for (auto x = 0; x < Subchunk::CHUNK_LENGTH; x++)
	{
		for (auto y = 0; y < Subchunk::CHUNK_LENGTH; y++)
		{
			for (auto z = 0; z < Subchunk::CHUNK_LENGTH; z++)
			{
				auto b = subchunk[x, y, z];

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
	world_normal_buffer.base();

	buffer world_index_buffer{ GL_ELEMENT_ARRAY_BUFFER, world_indices };


	std::vector<geo::vec4> skybox(verts.size());
	buffer sky_vertex_buffer{ GL_ARRAY_BUFFER, skybox };
	sky_vertex_buffer.add_attribute(4, GL_FLOAT, sizeof(geo::vec4), NULL);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	//const auto rotation = geo::quarter_pi<float>();
	//const auto t = geo::translate(geo::vec3{ 0.0f, 0.0f, 0.0f });
	//const auto r = geo::rotate(geo::identity<geo::mat4>(), rotation, geo::vec3{ 0.0f, 0.0f, 1.0f });
	//const auto s = geo::scale(geo::vec3{ 5.0, 0.5f, 5.0f });
	//const auto m = t * r * s;
	auto m = geo::identity(), sky_m = m, sky_mvp = m, sky_imvp = m;

	auto compute_p = [&](auto fov)
	{
		const auto rads = geo::radians(fov);
		return geo::perspective(fov, geo::native(WIDTH), geo::native(HEIGHT), 1.0f, 10000.0f);
	};

	auto fov = 90.0f;
	auto p = compute_p(fov);

	

	Camera camera{ { 50.0f, 0.0f, 50.0f }, geo::radians(180.0f), geo::radians(0.0f), 0.002f};

	// let's make sure our timer stuff fires initially
	auto timer = 0.0f; 

	
	world_index_buffer.bind();
	world_normal_buffer.bind();

	auto last_time = std::chrono::high_resolution_clock::now();
	auto last_update = last_time;

	while (true)
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE) == TRUE)
		{
			if (msg.message == WM_QUIT) [[unlikely]]
			{
				PostQuitMessage(0);
				return EXIT_SUCCESS;
			}
		
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		const auto current_time = std::chrono::high_resolution_clock::now();
		const auto delta_time = (current_time - last_time).count() / 1e9f;
		last_time = current_time;

		if (timer > 1.0)
		{
			const auto fps = static_cast<int>(1.0f / delta_time);
			const auto title = std::format(L"geo - {} FPS", fps);
			SetWindowText(_window.hwnd(), title.c_str());
			last_update = current_time;
		}

		_window.key_action(VkKeyScan('w'), [&]() { camera.vel() += camera.forward() * (CAMERA_SPEED * delta_time); });
		_window.key_action(VkKeyScan('s'), [&]() { camera.vel() -= camera.forward() * (CAMERA_SPEED * delta_time); });
		
		_window.key_action(VkKeyScan('d'), [&]() { camera.vel() += camera.right() * (CAMERA_SPEED * delta_time); });
		_window.key_action(VkKeyScan('a'), [&]() { camera.vel() -= camera.right() * (CAMERA_SPEED * delta_time); });
		
		_window.key_action(VK_SPACE, [&]() { camera.vel() += camera.up() * (CAMERA_SPEED * delta_time); });
		_window.key_action(VK_LSHIFT, [&]() { camera.vel() -= camera.up() * (CAMERA_SPEED * delta_time); });
		
		if (window::key_pressed(VK_MBUTTON))
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

		const auto v = geo::lookAt(camera.pos(), camera.pos() - camera.dir(), camera.up());
		const auto pv = p * v;


		glClear(GL_DEPTH_BUFFER_BIT);


		world_program.use();
		world_vertex_buffer.bind(GL_DYNAMIC_DRAW);

		for (auto i = 0; i < world_vertices.size(); i++)
		{
			world_vertices_transform[i].pos = pv * world_vertices[i].pos;
		}

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(world_indices.size()), GL_UNSIGNED_INT, nullptr);


		const auto base = geo::scale(geo::identity(), geo::extend(camera.pos(), 1.f))

		sky_m = geo::translation(geo::identity(), camera.pos()) * geo::scale(geo::identity(), 1000.0f);
		sky_mvp = pv * sky_m;
		sky_imvp = geo::inverse(sky_mvp);

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

		//glFinish();

		_window.swap();

		// TODO: resolve the timing stuff so this can go at the top of the loop with the rest of it
		timer = (current_time - last_update).count() / 1e9f;
	}
}