#include "generate_puzzles.hpp"

#include <json/json.h>

//////////////////////////////

typedef struct {
		int max_depth;
		int min_unknown;
		int max_unknown;
	} level_limits_t;


// FIXME: set those numbers more logically
const level_limits_t levels_limits[] = {
		{
			.max_depth = 0,
			.min_unknown = 40,
			.max_unknown = 45,
		},
		{
			.max_depth = 0,
			.min_unknown = 45,
			.max_unknown = 50,
		},
		{
			.max_depth = 1,
			.min_unknown = 55,
			.max_unknown = 70,
		}
	};

static_assert(sizeof(levels_limits)/sizeof(*levels_limits) == NB_LEVELS,
	"incorrect number of values in levels_limits");



using namespace std;

/////////////////////////////////////////////////
/////////////////////////////////////////////////

partial_grid_t convert_full_to_patrial_grid(full_grid_t full_grid)
{
	partial_grid_t partial_grid;

	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
		{
			uint8_t val = full_grid.squares[row-1][col-1];

			if( val == 0 )
				continue;

			// value was deduced
			if( partial_grid.get_square_value(row, col) != 0 )
				continue;

			partial_grid.set_square_value(row, col, val);

			uint8_t __attribute__ ((unused)) out;
			out = partial_grid.make_eliminations(row, col);

			assert( out != -1  &&  "the given full graph is invalid");
		}

	return partial_grid;
}

/////////////////////////////////////////////////

bool try_generate_puzzle(puzzle_t *puzzle, std::mt19937 *rng)
{
	// this array is used to choose
	// a random square
	array< pair<uint8_t, uint8_t>, 81 > squares_array;
	int position = 0;
	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
			squares_array[position++] = pair(row, col);


	// generate a full grid
	// at each step
	// 		if we reached maximum number of unknown vals
	// 		for the current level , move to the next one
	//
	// 		if there is no way to continue , increase depth
	// 		and eventually move to next level
	//

	int current_level = 0;
	int current_depth = 0;
	int current_nb_unknown = 0;

	full_grid_t current_grid = generate_random_full_grid(rng);

	puzzle->solution = current_grid;

	// if we try to remove a value
	// and it fails
	// no need to retry it until
	// depth is increased
	bool precious_squares[9][9] = {};

	while( current_level < NB_LEVELS )
	{

		level_limits_t current_limits = levels_limits[current_level];

		// go to next level if necessary:
		if( current_nb_unknown == current_limits.max_unknown ||
			current_depth      >  current_limits.max_depth )
		{
			// the grid is not valid
			if( current_nb_unknown < current_limits.min_unknown )
				return false;

			// store the grid
			// and go to next level
			puzzle->levels[current_level] = current_grid;

			current_level += 1;
			continue;
		}


		// try to remove a random square
		// shuffle for random order
		shuffle(squares_array.begin(), squares_array.end(), *rng);

		// this is used to know if the search
		// was successful
		bool removed_a_square = false;

		for( auto [row, col] : squares_array )
		{
			// if value was already removed
			// or if the square is precious
			// nothing to do
			if( current_grid.squares[row-1][col-1] == 0 ||
				precious_squares[row-1][col-1] == true )
				continue;

			// try to remove this square and see if
			// it can be solved without increasing difficulty
			full_grid_t F_grid_copy = current_grid;
			F_grid_copy.squares[row-1][col-1] = 0;

			partial_grid_t P_grid_copy = convert_full_to_patrial_grid(F_grid_copy);

			if( P_grid_copy.simplify_with_suppositions(current_depth) != -1 &&
				P_grid_copy.is_full() )
			{
				current_grid.squares[row-1][col-1] = 0;
				current_nb_unknown += 1;

				removed_a_square = true;
				break;
			}

			// this square can not be removed
			// mark it as precious
			precious_squares[row-1][col-1] = true;


		} // for( auto [row, col] : squares_array )


		// if no value is found increase depth
		if( ! removed_a_square )
		{
			current_depth += 1;

			for( uint8_t row = 1 ; row <= 9 ; row++ )
				for( uint8_t col = 1 ; col <= 9 ; col++ )
					precious_squares[row-1][col-1] = false;
		}


	} // while( current_level < NB_LEVELS )

	return true;
}

puzzle_t generate_puzzle(std::mt19937 *rng)
{
	puzzle_t puzzle;

	while( ! try_generate_puzzle(&puzzle, rng) )
		;

	return puzzle;
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////

void print_puzzle(puzzle_t puzzle)
{
	print_full_grid_t(puzzle.solution);
	for( full_grid_t grid : puzzle.levels )
		print_full_grid_t(grid);
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////

// this is used to represent how
// the squares of the grid are shuffled
//
// indexes start at 0
typedef array< array< pair<uint8_t, uint8_t>, 9>, 9> grid_shuffling_t;

grid_shuffling_t 
generate_grid_shuffling(mt19937 *rng);

// this is for debug only
void print_grid_shuffling(grid_shuffling_t);

// all the squares with values
// x will contain new_values[x-1]
void
apply_grid_shuffling(
	full_grid_t *grid,
	grid_shuffling_t *shuffling,
	std::array<uint8_t, 10> *new_values);

void randomize_puzzle(puzzle_t *puzzle, int level, int seed)
{
    mt19937 rng(seed);
	grid_shuffling_t grid_shuffling = generate_grid_shuffling(&rng);

	array<uint8_t, 10> new_values;
	for( uint8_t i = 0 ; i < 10 ; i++ )
		new_values[i] = i;
	
	// 0 is used for unknown values
	// so do not randomize it
	shuffle(new_values.begin()+1,
			new_values.end(),
			rng);

	apply_grid_shuffling(
		&puzzle->solution,
		&grid_shuffling,
		&new_values);
	
	apply_grid_shuffling(
		&(puzzle->levels[level]),
		&grid_shuffling,
		&new_values);
}


grid_shuffling_t 
generate_grid_shuffling(mt19937 *rng)
{
	// finding a shuffling means
	// finding a new order for
	// rows and cols. the same 
	// logic is used for both.
	//
	// first the order of the blocks
	// is set , then shuffle the
	// elements within each block

	grid_shuffling_t grid_shuffling;

	// reorder blocks
	array<uint8_t, 3> new_block_rows = {0,1,2};
	array<uint8_t, 3> new_block_cols = {0,1,2};

	shuffle(new_block_rows.begin(),
			new_block_rows.end(),
			*rng);

	shuffle(new_block_cols.begin(),
			new_block_cols.end(),
			*rng);



	array<uint8_t, 9> new_rows;
	array<uint8_t, 9> new_cols;

	for( uint8_t pos = 0 ; pos < 9 ; pos++ )
	{
		new_rows[pos] = ( pos % 3 ) +  3 * new_block_rows[pos/3];
		new_cols[pos] = ( pos % 3 ) +  3 * new_block_cols[pos/3];
	}

	// reorder rows of the same block
	// and cols of the same block
	for( uint8_t start = 0 ; start < 9 ; start += 3 )
	{
		shuffle(new_rows.begin()+start,
				new_rows.begin()+start+3,
				*rng);

		shuffle(new_cols.begin()+start,
				new_cols.begin()+start+3,
				*rng);
	}



	// set the positions
	// potentially doing a 
	// transpose
	bool transpose = (*rng)() % 2;

	for( uint8_t row = 0 ; row < 9 ; row++ )
		for( uint8_t col = 0 ; col < 9 ; col++ )
			if( transpose )
				grid_shuffling[col][row] = pair(new_rows[row],new_cols[col]);
			else
				grid_shuffling[row][col] = pair(new_rows[row],new_cols[col]);


	return grid_shuffling;
}

void
apply_grid_shuffling(
	full_grid_t *grid,
	grid_shuffling_t *shuffling,
	std::array<uint8_t, 10> *new_values)
{
	assert( (*new_values)[0] == 0 );

	full_grid_t grid_copy = *grid;

	for( uint8_t row = 0 ; row < 9 ; row++ )
		for( uint8_t col = 0 ; col < 9 ; col++ )
		{
			auto [new_row, new_col] = (*shuffling)[row][col];

			grid->squares[row][col] = (*new_values)[
										grid_copy.squares[new_col][new_row]
										];
		}
}

/////////////////////////////////////////////////


current_position_t
puzzle_to_current_position(puzzle_t *puzzle, int level)
{
	current_position_t position;

	position.solution = puzzle->solution;

	position.position = puzzle->levels[level];

	for( uint8_t row = 0 ; row < 9 ; row++ )
		for( uint8_t col = 0 ; col < 9 ; col++ )
			position.origins[row][col] = GRID_VALUE_ORIGIN_START;

	return position;
}



// this functions converts a 9 by 9 array
// into a json array of arrays
//
// json_arr must be an empty array
template<typename T>
void to_json_helper(Json::Value *json_arr, T (*arr)[9][9])
{
	assert( json_arr->isArray()  &&  json_arr->size() == 0 );

	for( uint8_t row = 0 ; row < 9 ; row++ )
	{
		(*json_arr)[row] = Json::arrayValue;

		for( uint8_t col = 0 ; col < 9 ; col++ )
			(*json_arr)[row][col] = (*arr)[row][col];
	}
}



char *current_position_to_json_string(current_position_t *position, bool include_solution)
{
	Json::Value json_position = Json::objectValue;

	Json::Value attr_solution = Json::arrayValue;
	Json::Value attr_position = Json::arrayValue;
	Json::Value attr_origins  = Json::arrayValue;


	to_json_helper( &attr_solution, &position->solution.squares );
	to_json_helper( &attr_position, &position->position.squares );
	to_json_helper( &attr_origins,  &position->origins );

	if( include_solution )
		json_position["solution"] = attr_solution;

	json_position["position"] = attr_position;
	json_position["origins"]  = attr_origins;
	

	Json::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "";
	string result = Json::writeString(wbuilder, json_position);

	return strdup(result.c_str());
}



/////////////////////////////////////////////////
/////////////////////////////////////////////////

void print_grid_shuffling(grid_shuffling_t grid_shuffling)
{
	printf("\n");
	for( auto row : grid_shuffling )
	{
		for( auto [x,y] : row )
			printf("(%d %d) ", x, y);

		printf("\n");
	}
}
