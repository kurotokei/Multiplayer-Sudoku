#ifndef _GENERATE_PUZZLE_HPP_
#define _GENERATE_PUZZLE_HPP_

#include "full_grid.hpp"
#include "partial_grid.hpp"

#define NB_LEVELS 3

extern "C" {

typedef enum {
		GRID_VALUE_ORIGIN_START,
		GRID_VALUE_ORIGIN_PLAYER_1,
		GRID_VALUE_ORIGIN_PLAYER_2
	} grid_value_origin_t;

typedef struct {
		full_grid_t solution;
		full_grid_t position;
		grid_value_origin_t origins[9][9];
	} current_position_t;


typedef struct {
		full_grid_t solution;
		full_grid_t levels[NB_LEVELS];
	} puzzle_t;

// applies transformations
// to the given puzzle
//
// in practice only one level will be used
// so no need to randomize the others
//
// a seed is given instead of rng to
// simplify the calls from python
void randomize_puzzle(puzzle_t *puzzle, int level, int seed);

current_position_t
puzzle_to_current_position(puzzle_t *puzzle, int level);


// return a string
// python must free it
char *current_position_to_json_string(current_position_t *position, bool include_solution);


} // extern "C"

// this is for debug only
void print_puzzle(puzzle_t puzzle);

// convert full_grid to partial_grid and
// call make eliminations
partial_grid_t convert_full_to_patrial_grid(full_grid_t full_grid);

// generate a full grid
// and try to use it to generate the puzzles
//
// return true if puzzle is generated
bool try_generate_puzzle(puzzle_t *puzzle, std::mt19937 *rng);

// calls try_generate_puzzle until
// sucess
puzzle_t generate_puzzle(std::mt19937 *rng);


#endif
