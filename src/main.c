#include <time.h>
#include <getopt.h>
#include "sim.h"
#include "error.h"

#define OUT_FILE "res" // Name of the output .dot file
#define HELP_MSG \
" Usage: sim [options] \n\
\n\
 Options with no argument:\n\
 --help,     -h          show this message\n\
 --time,     -t          measure the simulation runtime\n\
 --symbolic, -s          perform symbolic simulation if possible\n\
 \n\
 Options with a required argument:\n\
 --file,     -f          specify the input QASM file (default STDIN)\n\
 --nsamples, -n          specify the number of samples used for measurement (default 1024)\n\
 \n\
 Options with an optional argument:\n\
 --measure,  -m          perform the measure operations encountered in the circuit, \n\
                         optional arg specifies the file for saving the measurement result (default STDOUT)\n\
 \n\
 The MTBDD result is saved in the file 'res.dot'.\n"

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    FILE *measure_output = stdout;
    bool opt_infile = false;
    bool opt_time = false;
    bool opt_measure = false;
    bool opt_symbolic = false;
    unsigned long samples = 1024;
    
    int opt;
    static struct option long_options[] = {
        {"help",     no_argument,        0, 'h'},
        {"time",     no_argument,        0, 't'},
        {"file",     required_argument,  0, 'f'},
        {"measure",  optional_argument,  0, 'm'},
        {"nsamples", required_argument,  0, 'n'},
        {"symbolic", no_argument,        0, 's'},
        {0, 0, 0, 0}
    };

    char *endptr;
    while((opt = getopt_long(argc, argv, "htf:m::n:s", long_options, 0)) != -1) {
        switch(opt) {
            case 'h':
                printf("%s\n", HELP_MSG);
                exit(0);
            case 't':
                opt_time = true;
                break; 
            case 'f':
                opt_infile = true;
                input = fopen(optarg, "r");
                if (input == NULL) {
                    error_exit("Invalid input file.");
                }
                break;
            case 'm':
                opt_measure = true;
                if (!optarg && optind < argc && argv[optind][0] != '-') {
                    optarg = argv[optind++];
                    measure_output = fopen(optarg, "w");
                    if (measure_output == NULL) {
                        error_exit("Invalid output file.");
                    }
                }
                break;
            case 'n':
                samples = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    error_exit("Invalid number of samples.");
                }
                break;
            case 's':
                opt_symbolic = true;
                break;
            case '?':
                exit(1); // error msg already printed by getopt_long
        }
    }

    init_sylvan();
    init_my_leaf();
    if (opt_symbolic) {
        init_my_leaf_symb_val();
        init_my_leaf_symb_map();
    }
    FILE *out = fopen(OUT_FILE".dot", "w");
    if (out == NULL) {
        error_exit("Cannot open output file.");
    }
    MTBDD circ;
    int *bits_to_measure;
    bool is_measure = false;
    int n_qubits;

    struct timespec t_start, t_finish;
    double t_el;
    clock_gettime(CLOCK_MONOTONIC, &t_start); // Start the timer

    sim_file(input, &circ, &n_qubits, &bits_to_measure, &is_measure, opt_symbolic);

    if (opt_measure && is_measure) {
        measure_all(samples, measure_output, circ, n_qubits, bits_to_measure);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_finish); // End the timer
    
    mtbdd_fprintdot(out, circ);

    circuit_delete(&circ);
    stop_sylvan();
    fclose(out);
    if (opt_infile) {
        fclose(input);
    }

    t_el = t_finish.tv_sec - t_start.tv_sec + (t_finish.tv_nsec - t_start.tv_nsec) * 1.0e-9;

    if (opt_time) {
        printf("Time=%.3gs\n", t_el);
    }

    return 0;
}