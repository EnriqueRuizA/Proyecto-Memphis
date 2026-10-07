// src/util.cpp — RNG global (definición de util.h)
#include "util.h"

std::mt19937 g_rng(std::random_device{}());
