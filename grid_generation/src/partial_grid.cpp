#include "partial_grid.hpp"

using namespace std;

/////////////////////////////////////////////////

uint8_t partial_grid_square_t::get_value(void)
{
	return this->value;
}

void partial_grid_square_t::set_value(uint8_t val)
{
	assert( 1 <= val && val <= 9 );
	assert( this->get_can_be(val) );

	this->value = val;
}

bool partial_grid_square_t::get_can_be(uint8_t val)
{
	// can just return false for others
	// but this will catch errors so keep it
	assert( 1 <= val && val <= 9 );

	// using get_can_be when value is known
	// is probably an error
	assert( this->get_value() == 0 );

	return ( this->possible_values >> (9-val) ) & 1;
}

void partial_grid_square_t::set_can_not_be(uint8_t val)
{
	// can just ignore for others
	// but this will catch errors so keep it
	assert( 1 <= val && val <= 9 );

	// using set_can_not_be when value is known
	// is probably an error
	assert( this->get_value() == 0 );

	uint16_t mask = 1 << (9-val);
	this->possible_values &= ~mask;
}

uint8_t partial_grid_square_t::get_nb_possible_values(void)
{
	// TODO: use bitshifts to make it faster
	uint8_t tot = 0;
	for( uint8_t val = 1 ; val <= 9 ; val++ )
		tot += this->get_can_be(val);

	return tot;
}



/////////////////////////////////////////////////
/////////////////////////////////////////////////

uint8_t partial_grid_t::get_square_value(uint8_t row, uint8_t col)
{
	return this->squares[row-1][col-1].get_value();
}

void partial_grid_t::set_square_value(uint8_t row, uint8_t col, uint8_t val)
{
	this->squares[row-1][col-1].set_value(val);
}

bool partial_grid_t::get_square_can_be(uint8_t row, uint8_t col, uint8_t val)
{
	return this->squares[row-1][col-1].get_can_be(val);
}

void partial_grid_t::set_square_can_not_be(uint8_t row, uint8_t col, uint8_t val)
{
	this->squares[row-1][col-1].set_can_not_be(val);
}

uint8_t partial_grid_t::get_square_nb_possible_values(uint8_t row, uint8_t col)
{
	return this->squares[row-1][col-1].get_nb_possible_values();
}


bool partial_grid_t::is_full(void)
{
	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
			if( this->get_square_value(row, col) == 0 )
				return false;

	return true;
}


// return the block that contains 
// the given square
std::pair<uint8_t, uint8_t>
partial_grid_t::get_block_of_square(uint8_t row, uint8_t col)
{
	uint8_t block_row = 1 + (row-1)/3;
	uint8_t block_col = 1 + (col-1)/3;

	return pair(block_row, block_col);
}

// used to get the squares of a block
std::array< std::pair<uint8_t, uint8_t>, 9 >
partial_grid_t::get_squares_of_block(std::pair<uint8_t, uint8_t> block)
{
	std::array< std::pair<uint8_t, uint8_t>, 9 > block_squares;

	uint8_t row_start = 1 + 3 * (block.first-1);
	uint8_t col_start = 1 + 3 * (block.second-1);

	uint8_t pos = 0;
	for( uint8_t row = row_start ; row < row_start + 3 ; row++ )
		for( uint8_t col = col_start ; col < col_start + 3 ; col++ )
			block_squares[pos++] = pair(row, col);


	return block_squares;
}

std::array< std::pair<uint8_t, uint8_t>, 9 >
partial_grid_t::get_squares_of_row(uint8_t row)
{
	assert( 1 <= row  &&  row <= 9 );

	std::array< std::pair<uint8_t, uint8_t>, 9 > row_squares;
	for( uint8_t i = 0 ; i < 9 ; i++ )
		row_squares[i] = pair(row, i+1);

	return row_squares;
}

std::array< std::pair<uint8_t, uint8_t>, 9 >
partial_grid_t::get_squares_of_col(uint8_t col)
{
	assert( 1 <= col  &&  col <= 9 );

	std::array< std::pair<uint8_t, uint8_t>, 9 > col_squares;
	for( uint8_t i = 0 ; i < 9 ; i++ )
		col_squares[i] = pair(i+1, col);

	return col_squares;
}

/////////////////////////////////////////////////

int partial_grid_t::make_eliminations_on_square(
			uint8_t row,
			uint8_t col,
			uint8_t forbidden_val)
{
	// if a values is known skip it
	if( this->get_square_value(row, col) != 0 )
		return 0;

	this->set_square_can_not_be(row, col, forbidden_val);

	uint8_t nb_pos = this->get_square_nb_possible_values(row, col);

	// if there are no possible values
	// it is an error
	if( nb_pos == 0 )
		return -1;

	// there is nothing left to do
	// and no new value was found
	if( nb_pos > 1 )
		return 0;


	// there is exactly one possible value left
	// so set it and recurse
	
	// find the only possible value
	uint8_t unique_val;
	for( unique_val = 1 ; unique_val <= 9 ; unique_val++ )
	{
		if( this->get_square_can_be(row, col, unique_val) )
			break;
	}

	this->set_square_value(row, col, unique_val);
	int nb_new_values = this->make_eliminations(row, col);

	if( nb_new_values == -1 )
		return -1;
	
	// +1 to count (row,col)
	return nb_new_values+1;
}


int partial_grid_t::make_eliminations(uint8_t last_row, uint8_t last_col)
{
	uint8_t val = this->get_square_value(last_row, last_col);

	assert( val != 0 );

	// nb new values
	int found = 0;

	array<pair<uint8_t, uint8_t>, 9> same_block =
			this->get_squares_of_block(
				this->get_block_of_square(last_row, last_col)
			);

	array<pair<uint8_t, uint8_t>, 27> squares_to_check;
	copy(same_block.begin(), same_block.end(), squares_to_check.begin());

	for( uint8_t position = 1, idx = 9 ; position <= 9 ; position++ )
	{
		squares_to_check[idx++] = pair(last_row, position);
		squares_to_check[idx++] = pair(position, last_col);
	}


	// do the exact same logic for rows and cols
	for( auto [row, col] : squares_to_check )
	{
		int nb_new = make_eliminations_on_square(row, col, val);
		if( nb_new == -1 )
			return -1;
		found += nb_new;
	}

	return found;
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

int
partial_grid_t::get_deduced_values(
		std::array< std::pair<uint8_t, uint8_t>, 9 > squares_group,
		std::array< uint8_t, 9 > *deduced_values
	)
{

	// if a value is already present
	// no need to try to deduce it
	array< bool, 9 > value_is_present = {};


	// for every value from 1 to 9
	// store nb times values was seen
	// and index of last time it was seen
	//
	// at the end if seen exactly once we
	// can deduce the value of the square
	typedef struct {
				uint8_t counter;
				uint8_t index;
		} value_counter_t;

	array< value_counter_t, 9 > value_counters = {};

	for( uint8_t input_idx = 0 ; input_idx < 9 ; input_idx++ )
	{
		auto [row,col] = squares_group[input_idx];

		// if the value of a square is known
		// do not try to make deductions
		uint8_t square_value = this->get_square_value(row, col);
		if( square_value != 0 )
		{
			value_is_present[square_value - 1] = true;
			continue;
		}

		for( uint8_t value = 1 ; value <= 9 ; value++ )
			if( ! value_is_present[value-1]  &&  
				this->get_square_can_be(row, col, value) )
			{
				value_counters[value-1].index = input_idx;
				value_counters[value-1].counter += 1;
			}
	}

	
	// initnialize all the values to 0
	fill( deduced_values->begin(), deduced_values->end(), 0);

	for( uint8_t value = 1 ; value <= 9 ; value++ )
	{
		// if value is present
		// do not try to make deductions
		if( value_is_present[value-1] )
			continue;

		auto v_counter = value_counters[value-1];

		// here there a value needed but no
		// square available for it
		if( v_counter.counter == 0 )
			return -1;

		// there is exactly one square for this
		//
		if( v_counter.counter == 1 )
		{
			// if only square is already used
			// it means there is no solution
			if( (*deduced_values)[v_counter.index] > 0 )
				return -1;

			(*deduced_values)[v_counter.index] = value;
		}
	}

	return 0;
}

int partial_grid_t::make_deductions_on_group(
		std::array< std::pair<uint8_t, uint8_t>, 9 > squares_group
	)
{
	int found = 0;

	std::array< uint8_t, 9 > new_values;
	if( this->get_deduced_values(squares_group, &new_values) == -1 )
		return -1;

	for( auto [square, value] : views::zip(squares_group, new_values) )
	{
		if( value == 0 )
			continue;

		// this an happend if a previously set value
		// was able to deduce this oe using
		// elimination , in that case nothing to do
		if( this->get_square_value(square.first, square.second) != 0 )
			continue;

		// check that the deduced values is still valid
		// this can happend if a previously
		// deduced value causes eliminations
		// that make this one invalid
		if( ! this->get_square_can_be(square.first, square.second, value) )
			return -1;

		// set the value and 
		// make eliminations
		this->set_square_value(square.first, square.second, value);
		found += 1;

		int nb_new_vals = this->make_eliminations(square.first, square.second);

		if( nb_new_vals == -1 )
			return -1;
		found += nb_new_vals;
	}

	return found;
}

int partial_grid_t::make_deductions(void)
{
	// nb values found
	int found = 0;

	// the same logic is used on all 
	// rows,cols,blocks
	
	for( uint8_t row = 1 ; row <= 9 ; row++ )
	{
		int nb_new_vals = this->make_deductions_on_group(
								this->get_squares_of_row(row));
	
		if( nb_new_vals == -1 )
			return -1;

		found += nb_new_vals;
	}

	for( uint8_t col = 1 ; col <= 9 ; col++ )
	{
		int nb_new_vals = this->make_deductions_on_group(
								this->get_squares_of_col(col));
	
		if( nb_new_vals == -1 )
			return -1;

		found += nb_new_vals;
	}

	for( uint8_t i = 1 ; i <= 3 ; i++ )
		for( uint8_t j = 1 ; j <= 3; j++ )
		{
			pair<uint8_t, uint8_t> block = {i, j};

			int nb_new_vals = this->make_deductions_on_group(
									this->get_squares_of_block(block));
	
			if( nb_new_vals == -1 )
				return -1;

			found += nb_new_vals;
		}

	return found;
}


int partial_grid_t::simplify(void)
{
	int found = 0;
	while(true)
	{
		int nb_new_vals = 0;

		nb_new_vals = this->make_deductions();

		if( nb_new_vals == -1 )
			return -1;

		if( nb_new_vals == 0 )
			return found;
		
		found += nb_new_vals;
	}
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

int partial_grid_t::update_after_eliminations(void)
{

	int found = 0;

	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
		{
			// value is already known
			if( this->get_square_value(row, col) != 0 )
				continue;

			// no possible value is an error
			if( this->get_square_nb_possible_values(row, col) == 0 )
				return -1;

			// there is more than one possible value
			// so nothing to do
			if( this->get_square_nb_possible_values(row, col) > 1 )
				continue;

			uint8_t unique_val;
			for( unique_val = 1 ; unique_val <= 9 ; unique_val++ )
				if( this->get_square_can_be(row, col, unique_val) )
					break;

			this->set_square_value(row, col, unique_val);
			found += 1;
			int nb_new_vals = this->make_eliminations(row, col);

			if( nb_new_vals == -1 )
				return -1;

			found += nb_new_vals;
		}
	
	return found;
}

//////////////////////////////////
//////////////////////////////////


int partial_grid_t::make_eliminations_with_suppositions(int max_depth)
{
	assert( max_depth >= 1 );

	int nb_eliminations = 0;

	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
		{
			// if values is known nothing to do
			if( this->get_square_value(row, col) != 0 )
				continue;

			for( uint8_t val = 1 ; val <= 9 ; val++ )
			{
				if( ! this->get_square_can_be(row, col, val) )
					continue;

				partial_grid_t grid = *this;
				grid.set_square_value(row, col, val);

				if( grid.simplify_with_suppositions(max_depth-1) == -1 )
				{
					this->set_square_can_not_be(row, col, val);
					nb_eliminations += 1;
				}
			}

			// the case with one possiblity
			// is handled by simplify_with_suppositions
			if( this->get_square_nb_possible_values(row, col) == 0 )
				return -1;

		}

	return nb_eliminations;
}

int partial_grid_t::simplify_with_suppositions(int max_depth)
{
	assert( max_depth >= 0 );

	if( max_depth == 0 )
		return this->simplify();

	int found = 0;

	// while there are new results
	// 		simplify with depth-1
	// 		eliminate with depth
	// 
	while(true)
	{
		int nb_new_vals = this->simplify_with_suppositions(max_depth-1);


		if( nb_new_vals == -1 )
			return -1;

		found += nb_new_vals;


		int nb_new_elims = this->make_eliminations_with_suppositions(max_depth);

		if( nb_new_elims == -1 )
			return -1;

		// if no deductions where made then return
		if( nb_new_elims == 0 )
			return found;
		
		// new deductions where made
		// look if some new values can be found

		nb_new_vals = this->update_after_eliminations();

		if( nb_new_vals == -1 )
			return -1;

		found += nb_new_vals;
	}
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

// keep this function for unit tests later
bool init_grid(partial_grid_t *grid, uint8_t S[9][9])
{

	for( uint8_t row = 1 ; row <= 9 ; row++ )
		for( uint8_t col = 1 ; col <= 9 ; col++ )
		{
			if( grid->get_square_value(row, col) != 0 ||
				S[row-1][col-1] == 0 )
				continue;

			grid->set_square_value(row, col, S[row-1][col-1]);
			if( grid->make_eliminations(row, col) == -1 )
				return false;
		}

	return true;
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

void partial_grid_t::print(void)
{
	printf("\n");
	for( int row = 1 ; row <= 9 ; row++ )
	{
		for( int col = 1 ; col <= 9 ; col++ )
		{
			printf("%d ", this->get_square_value(row, col));
			if( col % 3 == 0 )
				printf(" ");
		}

		if( row % 3 == 0 )
			printf("\n");

		printf("\n");
	}
}

