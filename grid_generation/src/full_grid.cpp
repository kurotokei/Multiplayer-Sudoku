#include "full_grid.hpp"

using namespace std;

// this a helper function
// used by try_generate_random_full_grid
// it generates a signle row
// return false if it was not able to generate the row
//
// row starts at 0
static
bool
gen_full_grid_row(full_grid_t *grid, int row, mt19937 *rng);


/////////////////////////////////////////////////
/////////////////////////////////////////////////

void print_full_grid_t(full_grid_t grid)
{
	printf("\n");
	for( int i = 0 ; i < 9 ; i++ )
	{
		for( int j = 0 ; j < 9 ; j++ )
		{
			printf("%d ", grid.squares[i][j]);
			if( j%3 == 2 )
				printf(" ");
		}

		if( i%3 == 2 )
			printf("\n");

		printf("\n");
	}
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

full_grid_t
generate_random_full_grid(mt19937 *rng)
{

	while(true)
	{
		full_grid_t grid;

		// because of transoformation used later
		// there is no need to randomize the first row
		for( int i = 0 ; i < 9 ; i++ )
			grid.squares[0][i] = i+1;


		// try to generate the rows
		bool failed = false;
		for( int r = 1; r < 9 ; r++ )
			if( ! gen_full_grid_row(&grid, r, rng) )
			{
				failed = true;
				break;
			}

		if( ! failed )
			return grid;
	}
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

static
bool
gen_full_grid_row(full_grid_t *grid, int row, mt19937 *rng)
{

	// for every square store possible values in that square
	// keep track of remaining values 
	// start from left
	// if there are remainig values possible for this square:
	// 		choose one randomly and set it
	// else:
	// 		for rem_val in remaining values ( in random order )
	// 			try to find a previous valus to do a swap
	// 		
	// 		if no swap is possible , then there is no way to continue
	// 			return false
	//


	// store pos values for each col
	bool row_pos_vals[9][9];


	// initialize to true
	for( int col = 0 ; col < 9 ; col++ )
		for( int pos = 0 ; pos < 9 ; pos++ )
			row_pos_vals[col][pos] = true;

	// remove values in same column
	for( int col = 0 ; col < 9 ; col++ )
		for( int r = 0 ; r < row ; r++ )
		{
			int8_t forbidden = grid->squares[r][col]; 
			row_pos_vals[col][forbidden-1] = false;
		}


	// remove values in same block
	int block_row_start = 3 * (row / 3);
	for( int prev_row = block_row_start ; prev_row < row ; prev_row++ )
	{
		for( int block = 0 ; block < 3 ; block++ )
		{
			int col_start = 3*block;
			int col_end   = 3*block+3;

			for( int prev_col = col_start ; prev_col < col_end ; prev_col++ )
			{
				int8_t forbidden = grid->squares[prev_row][prev_col];

				for( int col = col_start ; col < col_end ; col++ )
					row_pos_vals[col][forbidden-1] = false;
			}
		}
	}

	// store remaining values to set
	// initialize to true
	bool row_remaining[9];

	for( int i = 0 ; i < 9 ; i++ )
		row_remaining[i] = true;


	for( int col = 0 ; col < 9 ; col++ )
	{
		// find possible values for this square
		// it must be a possible one and not yet set
		vector<int8_t> col_pos_values = {};

		for( int i = 0 ; i < 9 ; i++ )
			if( row_remaining[i]  &&  row_pos_vals[col][i] )
				col_pos_values.push_back(i+1);

		// if at least one value is found
		// choose one randomly and set it
		if( col_pos_values.size() != 0 )
		{
			int8_t val = col_pos_values[ (*rng)() % col_pos_values.size() ];
			grid->squares[row][col] = val;

			row_remaining[val-1] = false;
			continue;
		}

		// no value is found, try to swap with
		// previously set value
		// col_pos_prevs contains possible indexes

		// try all remaining ones in random order
		vector<int8_t> row_remaining_vector;
		for( int i = 0 ; i < 9 ; i++ )
			if( row_remaining[i] )
				row_remaining_vector.push_back(i+1);

		shuffle(row_remaining_vector.begin(),
				row_remaining_vector.end(),
				*rng);

		// set to true if a value is found
		bool val_was_set = false;
		for( int8_t val : row_remaining_vector )
		{
			vector<int8_t> col_pos_prevs = {};

			for( int8_t prev = 0 ; prev < col ; prev++ )
				if( row_pos_vals[col][-1 + grid->squares[row][prev]] &&
					row_pos_vals[prev][val-1] )
					col_pos_prevs.push_back(prev);

			// no previous element is available for swap
			if( col_pos_prevs.size() == 0 )
				continue;

			// val will be placed , so remove it from remainings :
			row_remaining[val-1] = false;

			// choose a random index from possible
			// previous indexes
			int8_t prev = col_pos_prevs[ (*rng)() % col_pos_prevs.size() ];

			int8_t tmp = grid->squares[row][prev];
			grid->squares[row][prev] = val;
			grid->squares[row][col]  = tmp;

			val_was_set = true;
			break;
		}


		if( val_was_set )
			continue;

		// when this point is reached ,it means there is no way to continue
		return false;
	}

	return true;
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

bool
full_grid_is_valid(full_grid_t *grid)
{

	// for every element r,c
	// 		set val to true in its row,column and block
	// return true if at the end all values are set to true

	bool rows_values[9][9] = {};
	bool cols_values[9][9] = {};
	bool blocks_values[3][3][9] = {};


	for( int row = 0 ; row < 9 ; row++ )
		for( int col = 0 ; col < 9 ; col++ )
		{
			int8_t val = grid->squares[row][col];

			rows_values[row][val-1] = true;
			cols_values[col][val-1] = true;

			blocks_values[row/3]
						 [col/3]
						 [val-1] = true;
		}

	for( int i = 0 ; i < 9 ; i++ )
		for( int j = 0 ; j < 9 ; j++ )
			if( ! ( rows_values[i][j] &&
					cols_values[i][j] &&
					blocks_values[i/3][i%3][j] ) )
				return false;

	return true;
}
