#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef rv_sizeof_array
#define rv_sizeof_array(x) sizeof(x) / sizeof(x[0])
#endif

#ifdef _WIN32
#define RV_EXPORT __declspec(dllexport)
#else
#define RV_EXPORT __attribute__((visibility("default")))
#endif

#define RV_PLUGIN
