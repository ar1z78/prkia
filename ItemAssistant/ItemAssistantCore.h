#pragma once

// We are now building a single EXE, so we don't need DLL exports or imports.
// This makes the macro "invisible" to the compiler.
#define ITEMASSISTANTCORE_API 

// We also removed the #pragma comment(lib...) so the linker stops looking for the old DLL.
