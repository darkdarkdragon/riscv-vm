// See LICENSE for license details.

//**************************************************************************
// Median filter bencmark
//--------------------------------------------------------------------------
//
// This benchmark performs a 1D three element median filter. The
// input data (and reference data) should be generated using the
// median_gendata.pl perl script and dumped to a file named
// dataset1.h.

#include "util.h"

#include "median.h"

//--------------------------------------------------------------------------
// Input/Reference Data

#include "dataset1.h"

//--------------------------------------------------------------------------
// Main

int main( int argc, char* argv[] )
{
//  int results_data[DATA_SIZE];

#if PREALLOCATE
  // If needed we preallocate everything in the caches
//  median( DATA_SIZE, input_data, results_data );
#endif
  const int repeat = 1000;
  printf("STARTING with datasize=%d repeat=%d\n", DATA_SIZE, repeat);
  // Do the filter
  setStats(1);
  for (int j = 0; j < repeat; j++) {
    median( DATA_SIZE, input_data, results_data );
  }
  setStats(0);

  // Check the results
  // return verify( DATA_SIZE, results_data, verify_data );
  int res = verify( DATA_SIZE, results_data, verify_data );
  if (res != 0) {
    printf("COMPARISON FAILED at %d\n", res-1);
  }

}
