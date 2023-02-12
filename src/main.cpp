#pragma once

#include <iostream>
#include <run.h>
#include <signal.h>

using namespace std;


extern FILE *_debug;
extern struct env *global_env;

void handle_signal(int s) {
    printf("Received signal %d\n", s);

    cout << "L1 Misses " << global_env->l1_misses << endl;
    cout << "L2 Misses " << global_env->l2_misses << endl;
    cout << "L3 Misses " << global_env->l3_misses << endl;

    fflush(_debug);
    exit(1);
}

int main(int argc, char* argv[]) {

    // check for correct number of input arguments
    if (argc != 3) {
        cout << "Format: ./bin/simulator <trace file prefix> <num files>" << endl;
        exit(1);
    }

    int numtraces = atoi(argv[2]);

    for (int i =0; i < 19; i++)
        signal(i, handle_signal);
    
    printf("Starting simulator with inclusive policy\n");

    start_simulator(argv[1], numtraces, INCLUSIVE);
    // start_simulator(argv[1], numtraces, NINE);
    // start_simulator(argv[1], numtraces, EXCLUSIVE);

    return 1;  
}