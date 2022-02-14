// See LICENSE.SiFive for license details.
// See LICENSE.Berkeley for license details.

#if VM_TRACE
#include <memory>
#include "verilated_vcd_c.h"
#endif
#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>


// For option parsing, which is split across this file, Verilog, and
// FESVR's HTIF, a few external files must be pulled in. The list of
// files and what they provide is enumerated:
//
// $RISCV/include/fesvr/htif.h:
//   defines:
//     - HTIF_USAGE_OPTIONS
//     - HTIF_LONG_OPTIONS_OPTIND
//     - HTIF_LONG_OPTIONS
// $(ROCKETCHIP_DIR)/generated-src(-debug)?/$(CONFIG).plusArgs:
//   defines:
//     - PLUSARG_USAGE_OPTIONS
//   variables:
//     - static const char * verilog_plusargs


static uint64_t trace_count = 0;
bool verbose = true;
bool done_reset = false;


double sc_time_stamp()
{
  return trace_count;
}




int main(int argc, char** argv)
{
  unsigned random_seed = (unsigned)time(NULL) ^ (unsigned)getpid();
  uint64_t max_cycles = -1;
  int ret = 0;
  bool print_cycles = false;

#if VM_TRACE
  FILE * vcdfile = NULL;
  uint64_t start = 0;
  vcdfile = fopen("dump.vcd", "w");
#endif

  

  if (verbose)
    fprintf(stderr, "using random seed %u\n", random_seed);

  srand(random_seed);
  srand48(random_seed);

  Verilated::randReset(2);
  Verilated::commandArgs(argc, argv);
  TEST_HARNESS *tile = new TEST_HARNESS;

#if VM_TRACE
  Verilated::traceEverOn(true); // Verilator must compute traced signals
  std::unique_ptr<VerilatedVcdFILE> vcdfd(new VerilatedVcdFILE(vcdfile));
  std::unique_ptr<VerilatedVcdC> tfp(new VerilatedVcdC(vcdfd.get()));
  if (vcdfile) {
    tile->trace(tfp.get(), 99);  // Trace 99 levels of hierarchy
    tfp->open("");
  }
#endif


  // The initial block in AsyncResetReg is either racy or is not handled
  // correctly by Verilator when the reset signal isn't a top-level pin.
  // So guarantee that all the AsyncResetRegs will see a rising edge of
  // the reset signal instead of relying on the initial block.
  int async_reset_cycles = 2;

  // Rocket-chip requires synchronous reset to be asserted for several cycles.
  int sync_reset_cycles = 10;

  while (trace_count < max_cycles) {
    if (done_reset && tile->io_success)
      break;

    tile->clock = 0;
    tile->reset = trace_count < async_reset_cycles*2 ? trace_count % 2 :
      trace_count < async_reset_cycles*2 + sync_reset_cycles;
    done_reset = !tile->reset;
    tile->eval();
#if VM_TRACE
    bool dump = tfp && trace_count >= start;
    if (dump)
      tfp->dump(static_cast<vluint64_t>(trace_count * 2));
#endif

    tile->clock = trace_count >= async_reset_cycles*2;
    tile->eval();
#if VM_TRACE
    if (dump)
      tfp->dump(static_cast<vluint64_t>(trace_count * 2 + 1));
#endif
    trace_count++;
  }

#if VM_TRACE
  if (tfp)
    tfp->close();
  if (vcdfile)
    fclose(vcdfile);
#endif

  if (trace_count == max_cycles)
  {
    fprintf(stderr, "*** FAILED *** via trace_count (timeout, seed %d) after %ld cycles\n", random_seed, trace_count);
    ret = 2;
  }
  else if (verbose || print_cycles)
  {
    fprintf(stderr, "*** PASSED *** Completed after %ld cycles\n", trace_count);
  }

  if (tile) delete tile;
  return ret;
}
