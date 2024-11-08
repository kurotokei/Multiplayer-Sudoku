#ifndef _PARTIAL_GRID_HPP_
#define _PARTIAL_GRID_HPP_

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <array>
#include <random>
#include <vector>
#include <algorithm>
#include <ranges>


// this class is used to store info
// about one square
//
// bitfields are used to reduce space usage
// this implementation uses 2 bytes , using
// bool for pos values would use 10 bytes
//
// attributes:
//		possible_values:
//			1 bit for each value
//			from 1 to 9
//
//		value:
//			the values of this square
//			0 if the values is not yet set
//
// 3 bits are left unused for now
//
//
class partial_grid_square_t
{
	private:
		uint16_t value : 4 = 0;
		uint16_t possible_values : 9 = 0x1FF;

	public:

		uint8_t get_value(void);
		void set_value(uint8_t val);

		bool get_can_be(uint8_t val);
		void set_can_not_be(uint8_t val);

		uint8_t get_nb_possible_values(void);

		// this just for debugging
		void print(void) {
			printf("\n%d,%d,", value, possible_values);
			for( int i = 1 ; i <= 9 ; i++ )
				printf("%d", get_can_be(i));
		}
};


class partial_grid_t
{
	private:
		// this is private with functions that
		// for now simply call the methods
		// of partial_grid_square_t.
		//
		// it is better this way in case i
		// add checks and compute values
		// at each step.
		partial_grid_square_t squares[9][9];

	public:

		uint8_t get_square_value(uint8_t row, uint8_t col);
		void set_square_value(uint8_t row, uint8_t col, uint8_t val);

		bool get_square_can_be(uint8_t row, uint8_t col, uint8_t val);
		void set_square_can_not_be(uint8_t row, uint8_t col, uint8_t val);

		uint8_t get_square_nb_possible_values(uint8_t row, uint8_t col);

		// return true if the value
		// of every square is known
		bool is_full(void);


		// return the block that contains
		// the given square
		std::pair<uint8_t, uint8_t>
		get_block_of_square(uint8_t row, uint8_t col);

		// used to get the squares of a block
		std::array< std::pair<uint8_t, uint8_t>, 9 >
		get_squares_of_block(std::pair<uint8_t, uint8_t> block);

		// used to get the squares of a row
		std::array< std::pair<uint8_t, uint8_t>, 9 >
		get_squares_of_row(uint8_t row);

		// used to get the squares of a col
		std::array< std::pair<uint8_t, uint8_t>, 9 >
		get_squares_of_col(uint8_t col);

		// this is called by make_eliminations
		// on all concerned squares
		// having this separate function simplifies
		// the code a lot
		int make_eliminations_on_square(
				uint8_t row,
				uint8_t col,
				uint8_t forbidden_val);


		// this is called after a square is set to val
		// it updates pos values in same row and col
		// return number of new values found
		// return -1 if an error is found
		int make_eliminations(uint8_t last_row, uint8_t last_col);

		// in some cases it is better
		// to separate the eliminations logic
		// from the deduction logic
		// this method looks for squares with
		// exactly one possible value
		// set it and call make_eliminations
		// return number of values deduced
		// return -1 if there is an error
		int update_after_eliminations(void);


		// this is called by make_deductions
		// to work on a specific row,col,block
		// for each square:
		//  return the deduced value or 0
		//
		// return -1 if an error is found
		int
		get_deduced_values(
				std::array< std::pair<uint8_t, uint8_t>, 9 > squares_group,
				std::array< uint8_t, 9 > *deduced_values
			);

		// used by make_deductions to work
		// on a specific row,col,block
		int make_deductions_on_group(
				std::array< std::pair<uint8_t, uint8_t>, 9 > squares_group
			);



		// use direct deductive steps to find new values
		// when a new values is found, it calls make_eliminations
		// return number of values found
		// return -1 if an error is found
		int make_deductions(void);

		// use make_eliminations and make_deductions
		// to find as many values as possible
		// return number of values deduced
		// return -1 if there is an error
		int simplify(void);

		// this functions tries all possible values
		// and use simplify_with_suppositions
		// to find a contradiction
		// return number of eliminations done
		//  NOTE:
		//      NOT NUMBER OF NEW VALUES
		// return -1 if there is an error
		int make_eliminations_with_suppositions(int max_depth);


		// try values and use simplify
		// to find contradictions
		// return number of new values found
		// return -1 if there is an error
		int simplify_with_suppositions(int max_depth);



		// THIS IS FOR DEBBUGING 
		void print(void);
};


#endif
