#ifndef _FULL_GRID_HPP_
#define _FULL_GRID_HPP_

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <random>
#include <vector>
#include <algorithm>

#define __maybe_unused  __attribute__ ((unused))

// this is simple structure to store
// a full grid, the value 0 is used
// for values not yet assigned
typedef struct {
        int8_t squares[9][9];
    } full_grid_t;

void print_full_grid_t(full_grid_t grid);

// takes an rng as param to avoid recreating it mutliple times
// return a valid full grid
full_grid_t
generate_random_full_grid(std::mt19937 *rng);

// return true if the given full grid is valid
// assumes that all elements have valid values
bool
full_grid_is_valid(full_grid_t *grid);


#endif
