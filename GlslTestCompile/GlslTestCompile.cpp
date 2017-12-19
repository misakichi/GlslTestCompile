// GlslTestCompile.cpp: アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#define MAX_LOADSTRING 100

#pragma comment(lib, "opengl32.lib")

PFNGLCREATESHADERPROC				glCreateShader;
PFNGLSHADERSOURCEPROC				glShaderSource;
PFNGLCOMPILESHADERPROC				glCompileShader;
PFNGLDELETESHADERPROC				glDeleteShader;
PFNGLGETSHADERIVPROC				glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog;

PFNWGLCREATECONTEXTATTRIBSARBPROC	wglCreateContextAttribsARB;
// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
const CHAR* szWindowClass = "gtcDummy";



struct WGL_CONTEXT {
	HGLRC	glCtx;
	HGLRC	glCtx2;
	HDC		hdc;
	HBITMAP	hbmp;
};
enum ShaderType {
	Unknown,
	Vertex,
	TesCtrl,
	TesEvaluation,
	Geometry,
	Fragment,
	Compute,
};

static const GLenum s_shaderTypeGL[] = {
	0,
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
	GL_COMPUTE_SHADER
};

struct wglInitParam
{
	HWND hwnd;
	int major;
	int minor;
	bool esMode;
};

WGL_CONTEXT wglInit(wglInitParam& param)
{
	WGL_CONTEXT ret = {};
	auto wndHdc = GetDC(param.hwnd);
	ret.hdc = wndHdc;

	BITMAPINFOHEADER bih;
	memset(&bih, 0, sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = 32;
	bih.biHeight = 32;
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;

	void* pbits;
	ret.hbmp = CreateDIBSection(ret.hdc, (BITMAPINFO*)&bih, DIB_PAL_COLORS, &pbits, NULL, 0);

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	//要求されたフォーマットに最も近いピクセルフォーマットのインデックスを要求 
	int pixelFormat = ChoosePixelFormat(ret.hdc, &pfd);
	//ピクセルフォーマットの設定 
	SetPixelFormat(ret.hdc, pixelFormat, &pfd);

	if (SetPixelFormat(ret.hdc, pixelFormat, &pfd) == FALSE) {
		DeleteObject(ret.hbmp);
		DeleteDC(ret.hdc);
		return {};
	}

	SelectObject(ret.hdc, ret.hbmp);
	ret.glCtx = wglCreateContext(ret.hdc);
	wglMakeCurrent(ret.hdc, ret.glCtx);
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");	
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");

	if (param.esMode || param.major > 0) {
		ret.glCtx2 = ret.glCtx;
		if (param.major == 0) {
			const int  ctxInitParam[] = {
				WGL_CONTEXT_FLAGS_ARB,           0,
				WGL_CONTEXT_PROFILE_MASK_ARB,    param.esMode ? WGL_CONTEXT_ES2_PROFILE_BIT_EXT : WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0,
			};
			ret.glCtx = wglCreateContextAttribsARB(ret.hdc, ret.glCtx2, ctxInitParam);
		} else {
			const int  ctxInitParam[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB,   param.major,
				WGL_CONTEXT_MINOR_VERSION_ARB,   param.minor,
				WGL_CONTEXT_FLAGS_ARB,           0,
				WGL_CONTEXT_PROFILE_MASK_ARB,    param.esMode ? WGL_CONTEXT_ES2_PROFILE_BIT_EXT : WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0,
			};
			ret.glCtx = wglCreateContextAttribsARB(ret.hdc, ret.glCtx2, ctxInitParam);
		}
		wglMakeCurrent(ret.hdc, ret.glCtx);
	}
	printf("OpenGL Infomation\n"); 
	printf("Vender %s\n", glGetString(GL_VENDOR));
	printf("Version %s\n", glGetString(GL_VERSION));
	printf("Language %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Renderer %s\n", glGetString(GL_RENDERER));

	return ret;
}

void printUsage()
{
	printf("glsltc.exe <shadertype> <options> <file>\n");
	printf("<ShaderType>        :   Compile shader target shader type.\n");
	printf("                        vs,tcs,tes,gs,fs or cs\n");
	printf("<options>\n");
	printf("-Include<includepath>\n");
	printf("-Dname<=value>\n");
	printf("-glv<x.y>           :   (optional) select OpenGL Version\n");
	printf("                        (use wglCreateContextAttribsARB)\n");
	printf("-entry<function>    :   (optional) entry funtion name.(default=\"main\")\n");
	printf("-es                 :   (optional) use es context.\n");

}

std::string preprocess(
	const char* file,
	std::vector<std::string>			&includePaths,
	std::vector<std::string>			&ignorePreProcess,
	std::map<std::string, std::string>	&defines
);

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int main(int argc, const char* argv[])
{
	if (argc <= 2) {
		printUsage();
		return 0;
	}

	ShaderType shaderType = ShaderType::Unknown;
	auto typeStr = argv[1];
	if (!strcmp(typeStr, "vs")) {
		shaderType = ShaderType::Vertex;
	}else if (!strcmp(typeStr, "tcs")) {
		shaderType = ShaderType::TesCtrl;
	} else if (!strcmp(typeStr, "tes")) {
		shaderType = ShaderType::TesEvaluation;
	} else if (!strcmp(typeStr, "gs")) {
		shaderType = ShaderType::Geometry;
	} else if (!strcmp(typeStr, "fs")) {
		shaderType = ShaderType::Fragment;
	} else if (!strcmp(typeStr, "cs")) {
		shaderType = ShaderType::Compute;
	}
	if (shaderType == ShaderType::Unknown) {
		printf("invalid shader type <%s>\n", typeStr);
		return -1;
	}
	std::vector<std::string>			includePaths;
	std::map<std::string, std::string>	defines;

	std::vector<std::string> glslSource;
	wglInitParam initParam = {};
	auto arg = argv+1;
	auto argEnd = argv + argc;
	while (++arg < argEnd) {
		if (strncmp(*arg, "-glv", 4) == 0) {
			auto ver = *arg + 4;
			initParam.major = atoi(ver);
			ver = strchr(ver, '.');
			if (ver) {
				initParam.minor = atoi(ver);
				ver = strchr(ver, '.');
			}
		} else if (strncmp(*arg, "-es", 3) == 0) {
			initParam.esMode = true;
		} else if (strncmp(*arg, "-Include", 8) == 0) {
			std::string value = *arg + 8;
			//trim space
			while (value.length()>0 && *value.begin() == ' ') {
				value.erase(value.begin());
			}
			while (value.length()>0 && *(value.end() - 1) == ' ') {
				value.erase(value.end() - 1);
			}
			//trim
			if (value.length() > 2 && *value.begin() == '\"' && *(value.end() - 1) == '\"') {
				value = value.substr(1, value.length() - 2);
			}
			includePaths.push_back(value);

		} else if (strncmp(*arg, "-D", 2) == 0) {
			const char* def = *arg + 2;
			std::string value = "";
			std::string name = def;
			const char* eqPoint = strchr(def, '=');
			if (eqPoint) {
				value = eqPoint + 1;
				*((char*)eqPoint)= 0;
				name = def;
			}
			defines.insert({ name, value });
		} else {
			//file?
			struct stat st = {};
			if (stat(*arg,  &st) == 0) {
				auto code = std::move(preprocess(*arg, includePaths, std::vector<std::string>({ "#version" }), defines));
				glslSource.push_back(code);
			} else if (errno == ENOENT) {
				printf("%s not found.", *arg);
			} else  {
				printf("denyed access %s.", *arg);
			}
		}
	}
	
	auto hInstance = GetModuleHandle(NULL);
    MyRegisterClass(hInstance);
	HWND hWnd = CreateWindow(szWindowClass, "", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) {
		return FALSE;
	}
	UpdateWindow(hWnd);

	initParam.hwnd = hWnd;
	auto wglInfo = wglInit(initParam);

	
	for (auto& code : glslSource) {
		auto shader = glCreateShader(s_shaderTypeGL[shaderType]);
		auto src = (code).c_str();
		auto srcSize = int(code.size());
		glShaderSource(shader, 1, &src, &srcSize);
		glCompileShader(shader);

		int result;
		glGetShaderiv(shader, GL_SHADER_TYPE, &result);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &result);
		glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &result);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &result);


		if (result == GL_FALSE) {

			int logLen;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

			logLen++;
			char* log = new char[logLen];
			int length;
			glGetShaderInfoLog(shader, logLen, &length, log);

			printf("compile failed.\n");
			printf("%s\n", log);
			printf("~~~~~~~~~~~~~\n", log);
			printf("%s\n", src);

			delete[] log;
		} else {
			printf("compile success.\n");
		}
		glDeleteShader(shader);
	}

	DestroyWindow(hWnd);



    return 1;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = NULL;

    return RegisterClassEx(&wcex);
}


//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウの描画
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
