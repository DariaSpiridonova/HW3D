#pragma once

#include "scene/scene.h"

// the counter of the failed results
static int fails = 0;
// to avoid errors in single execution
#define CHECK(c) do { if (!(c)) { std::printf("  FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } } while (0)
// to align the error message
static void report(const char* name, int before) { std::printf("%-46s %s\n", name, fails == before ? "ok" : "FAILED"); }

using V3 = Vector<double, 3>;
using V2 = Vector<double, 2>;
using Tri3 = Triangle<double, 3>;
using Tri2 = Triangle<double, 2>;