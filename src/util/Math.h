#pragma once

// Copyright 2015 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include <cstdlib>
#include <random>

namespace payshares
{
double rand_fraction();

size_t rand_pareto(float alpha, size_t max);

extern std::default_random_engine gRandomEngine;
}
