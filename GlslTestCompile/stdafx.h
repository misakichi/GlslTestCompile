// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>
#include <fcntl.h> 

#include <d3dcompiler.h>

// C ランタイム ヘッダー ファイル
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <io.h>
#include <sys/stat.h>

#include <vector>
#include <map>
#include <stack>

#include "GlslTestCompile.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include "../src/wglext.h"
#include "../src/glext.h"

// TODO: プログラムに必要な追加ヘッダーをここで参照してください
