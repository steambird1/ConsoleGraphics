#pragma once
#define _CRT_SECURE_NO_WARNINGS

/*
Warning:
[EN] For all internal calcutions, HEIGHT (Row) FIRST!
x: Row, y: Column !!!

Mustn't use iostream in this library.
The comment that included "..." means the method is incomplete.

[ZH] 下标先行后列，x 为行，y 为列！
请勿使用 iostream。
*/

// Consider the version
#if !defined(_WIN32) && !defined(__linux__)
#error "Unsupported platform"
#endif

#include "scg-utility.h"
#include "scg-console.h"
#include "scg-graph.h"
#include "scg-controls.h"
