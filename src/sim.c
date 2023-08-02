#include "sim.h"

/**
 * Function for number parsing from the input file (reads the number from the input until the end character is encountered)
 */
static long parse_num(FILE *in, char end)
{
    int c;
    char num[NUM_MAX_LEN] = {0};
    long n;

    // Load number to string
    while ((c = fgetc(in)) != end) {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file.");
        }
        else if (!isdigit(c) && c != '-') {
            error_exit("Invalid format - not a valid number.");
        }
        else if (strlen(num) + 1 < CMD_MAX_LEN) {
            int *temp = &c;
            strncat(num, (char*)temp, 1);
        }
        else {
            error_exit("Invalid command.");
        }
    }

    // Convert to integer value
    char *ptr;
    errno = 0;
    n = strtol(num, &ptr, 10);
    if (num == ptr || errno != 0) {
        error_exit("Invalid format - not a valid number.");
    }
    return n;
}

/** 
 * Function for getting the next qubit index for the command on the given line.
 */
static uint32_t get_q_num(FILE *in)
{
    int c;
    unsigned long n;

    while ((c = fgetc(in)) != '[') {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file.");
        }
    }

    n = parse_num(in, ']');
    if (n > UINT32_MAX || n < 0) {
        error_exit("Invalid format - not a valid qubit identifier.");
    }

    return ((uint32_t)n);
}

/**
 * Returns the number of iterations, should be called when a for loop is encountered
 */
static uint32_t get_iters(FILE *in)
{
    int c;
    long start, end, step, iters;

    while ((c = fgetc(in)) != '[') {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file.");
        }
    }

    start = parse_num(in, ':');
    end = parse_num(in, ':');
    step = parse_num(in, ']'); //TODO: support floats?

    iters = (end + 1 - start) / step;
    if (iters > UINT32_MAX || iters < 0) {
        error_exit("Invalid number of loop iterations.");
    }
    
    return ((uint32_t) iters);
}

void sim_file(FILE *in, MTBDD *circ, int *n_qubits, int **bits_to_measure, bool *is_measure, bool opt_symb)
{
    //TODO: refactoring
    int c;
    char cmd[CMD_MAX_LEN];
    bool init = false;

    bool is_loop = false;
    fpos_t loop_start;
    mtbdd_symb_t symbc;
    uint32_t iters;

    while ((c = fgetc(in)) != EOF) {
        for (int i=0; i < CMD_MAX_LEN; i++) {
            cmd[i] = '\0';
        }

        while (isspace(c)) {
            c = fgetc(in);
        }

        if (c == EOF) {
            return;
        }

        // Skip one line comments
        if (c == '/') {
            if ((c = fgetc(in)) == '/') {
                while ((c = fgetc(in)) != '\n') {
                    if (c == EOF) {
                        return;
                    }
                }
                continue;
            }
            else {
                error_exit("Invalid command.");
            }
        }

        // Load the command
        do {
            if (c == EOF) {
                error_exit("Invalid format - reached an unexpected end of file.");
            }
            else if (strlen(cmd) + 1 < CMD_MAX_LEN) {
                int *temp = &c;
                strncat(cmd, (char *)temp, 1);
            }
            else {
                error_exit("Invalid command.");
            }
        } while (!isspace(c = fgetc(in)));

        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file.");
        }

        // Identify the command
        if (strcmp(cmd, "OPENQASM") == 0) {}
        else if (strcmp(cmd, "include") == 0) {}
        else if (strcmp(cmd, "creg") == 0) {} //TODO: check if is valid?
        else if (strcmp(cmd, "qreg") == 0) {
            uint32_t n = get_q_num(in);
            *n_qubits = (int)n;
            *bits_to_measure = malloc(n * sizeof(int));
            if (*bits_to_measure == NULL) {
                error_exit("Memory allocation error.");
            }
            for (int i=0; i < n; i++) {
                (*bits_to_measure)[i] = -1;
            }
            circuit_init(circ, n);
            mtbdd_protect(circ);
            init = true;
        }
        else if (init) {
            if (strcmp(cmd, "for") == 0) {
                iters = get_iters(in);
                if (iters == 0) {
                    // skip symbolic
                    while ((c = fgetc(in)) != '}') { //TODO: check for comments - shouldn't count commented }
                        if (c == EOF) {
                            error_exit("Invalid format - reached an unexpected end of file.");
                        }
                    }
                    continue;
                }
                while ((c = fgetc(in)) != '{') {
                    if (c == EOF) {
                        error_exit("Invalid format - reached an unexpected end of file.");
                    }
                }
                is_loop = true; // TODO: allow nested loops?
                symb_init(circ, &symbc);
                if (fgetpos(in, &loop_start) != 0) {
                    error_exit("Could not get the current position of the stream.");
                }
                continue; // ';' not expected
            }
            else if (strcmp(cmd, "}") == 0) {
                if (!is_loop) {
                    error_exit("Invalid loop syntax.");
                }
                if (!opt_symb) {
                    iters--;
                    if (!iters) {
                        is_loop = false;
                    }
                    else { // next iteration
                        if (fsetpos(in, &loop_start) != 0) {
                            error_exit("Could not set a new position of the stream.");
                        }
                    }
                }
                else {
                    if (symb_refine(&symbc)) {
                        // is final result
                        is_loop = false;
                        symb_eval(circ, &symbc, iters); // TODO: should request gc call?
                    }
                    else {
                        if (fsetpos(in, &loop_start) != 0) {
                            error_exit("Could not set a new position of the stream.");
                        }
                        cs_k_reset();
                    }
                }
                continue; // ';' not expected
            }
            else if (strcmp(cmd, "measure") == 0) {
                uint32_t qt = get_q_num(in);
                uint32_t ct = get_q_num(in);
                *is_measure = true;
                (*bits_to_measure)[qt] = ct;
            }
            else if (strcmp(cmd, "x") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? gate_symb_x(&symbc.val, qt) : gate_x(circ, qt);
            }
            else if (strcmp(cmd, "y") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_y(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "z") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_z(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "h") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? gate_symb_h(&symbc.val, qt) : gate_h(circ, qt);
            }
            else if (strcmp(cmd, "s") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_s(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "t") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_t(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "rx(pi/2)") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_rx_pihalf(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "ry(pi/2)") == 0) {
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_ry_pihalf(circ, qt); //TODO:
            }
            else if (strcmp(cmd, "cx") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_cnot(circ, qt, qc); //TODO:
            }
            else if (strcmp(cmd, "cz") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_cz(circ, qt, qc); //TODO:
            }
            else if (strcmp(cmd, "ccx") == 0) {
                uint32_t qc1 = get_q_num(in);
                uint32_t qc2 = get_q_num(in);
                uint32_t qt = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_toffoli(circ, qt, qc1, qc2); //TODO:
            }
            else if (strcmp(cmd, "cswap") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt1 = get_q_num(in);
                uint32_t qt2 = get_q_num(in);
                (opt_symb && is_loop)? error_exit("Gate does not support symbolic simulation") : gate_fredkin(circ, qt1, qt2, qc); //TODO:
            }
            else {
                error_exit("Invalid command.");
            }
        }
        else {
            error_exit("Circuit not initialized.");
        }

        // Skip all remaining characters on the currently read line
        while ((c = fgetc(in)) != ';') {
            if (c == EOF) {
                error_exit("Invalid format - reached an unexpected end of file.");
            }
        }
    } // while
}

void measure_all(unsigned long samples, FILE *output, MTBDD circ, int n, int *bits_to_measure)
{
    prob_t random;
    prob_t p_qt_is_one;
    prob_t norm_coef;
    char curr_state[n+1];
    curr_state[n] = '\0';
    int curr_ct;

    htab_t *state_table = htab_init(n*n); //TODO: is optimal?
    
    for (unsigned long i=0; i < samples; i++) {
        norm_coef = 1;
        for (int j=0; j < n; j++) {
            curr_state[j] = 'x';
        }

        for (int j=0; j < n; j++) {
            curr_ct = bits_to_measure[j];
            if (curr_ct == -1) {
                continue;
            }

            p_qt_is_one = measure(&circ, j, curr_state, n) * norm_coef * norm_coef;
            random = (prob_t)rand() / RAND_MAX;
            if (random <= p_qt_is_one) {
                curr_state[curr_ct] = '1';
                norm_coef *= sqrt(1/p_qt_is_one);
            }
            else {
                curr_state[curr_ct] = '0';
                norm_coef *= sqrt(1/(1-p_qt_is_one));
            }
        }
        htab_lookup_add(state_table, curr_state);
    }
    htab_print_all(state_table, output);
    htab_free(state_table);
}

/* end of "sim.c" */