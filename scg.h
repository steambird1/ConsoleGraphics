#pragma once
#define _CRT_SECURE_NO_WARNINGS

/*
Warning:
[EN] For all internal calcutions, HEIGHT (Row) FIRST!
x: Row, y: Column !!!

Mustn't use iostream in this library.
The comment that included "..." means the method is incomplete.

[ZH] �±����к��У�x Ϊ�У�y Ϊ�У�
����ʹ�� iostream��
*/

// Consider the version
#if !defined(_WIN32) && !defined(__linux__)
#error "Unsupported platform"
#endif

#include "scg-utility.h"
#include "scg-console.h"
#include "scg-graph.h"
#include "scg-controls.h"
