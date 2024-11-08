#include <stdio.h>
#include <omp.h>

#include "full_grid.hpp"
#include "partial_grid.hpp"
#include "generate_puzzles.hpp"

using namespace std;

#define NB_THREADS 2
#define NB_PUZZLES 100

const char *data_file_name = "./build/sudoku_puzzles.data";

int main(__maybe_unused int argc, __maybe_unused char **argv)
{

	FILE *data_file = fopen(data_file_name, "wb");

	if( data_file == NULL )
	{
		perror("data file fopen: ");
		exit(1);
	}

	// keep the seeds to reproduce bugs
    random_device rd;
	volatile unsigned int seeds[NB_THREADS];
	for( int i = 0 ; i < NB_THREADS ; i++ )
		seeds[i] = rd();


	omp_set_num_threads(NB_THREADS);

	#pragma omp parallel
	{
		volatile int id = omp_get_thread_num();

		mt19937 rng(seeds[id]);

		#pragma omp for
		for( int i = 0 ; i < NB_PUZZLES ; i++ )
		{
			puzzle_t P = generate_puzzle(&rng);

			#pragma omp critical
			{
				if( 1 != fwrite( &P,
								  sizeof(P),
								  1,
								  data_file) )
				{
					perror("error while writing : ");
					exit(1);
				}
			}
		}

	}
	
	return 0;
}

