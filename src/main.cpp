#pragma once

#include <iostream>
#include<unordered_map>
#include <run.h>
#include <signal.h>

using namespace std;

unordered_map<string, int> tracefiles = {
    {"bzip2.log_l1misstrace", 2},
    {"gcc.log_l1misstrace", 2},
    {"gromacs.log_l1misstrace", 1},
    {"h264ref.log_l1misstrace", 1},
    {"hmmer.log_l1misstrace", 1},
    {"sphinx3.log_l1misstrace", 2},    
};

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
    // if (argc != 3) {
    //     cout << "Format: ./bin/simulator <trace file prefix> <num files>" << endl;
    //     exit(1);
    // }

    for (int i =0; i < 19; i++)
        signal(i, handle_signal);

    for(auto kvtraces: tracefiles){

        // int numtraces = atoi(argv[2]);
        int numtraces = kvtraces.second;

        printf("Starting simulator with inclusive policy\n");

        start_simulator((char*)("./traces/" + kvtraces.first).c_str(), numtraces, INCLUSIVE);
        // start_simulator(argv[1], numtraces, INCLUSIVE);
        // start_simulator(argv[1], numtraces, NINE);
        // start_simulator(argv[1], numtraces, EXCLUSIVE);
    }
    return 1;  
}