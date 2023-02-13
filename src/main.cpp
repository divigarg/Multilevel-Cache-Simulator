#pragma once

#include <iostream>
#include<map>
#include <run.h>
#include <signal.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

map<string, int> tracefiles = {
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

    for (int i =0; i < 19; i++)
        signal(i, handle_signal);

    for(auto kvtraces: tracefiles){
        int numtraces = kvtraces.second;
        time_point<system_clock> sTime, eTime;
        sTime = system_clock::now(); 
        start_simulator((char*)("./traces/" + kvtraces.first).c_str(), numtraces, INCLUSIVE);
        eTime = system_clock::now();

        duration<double> timeTaken = eTime-sTime;
        printf("Elapsed Time: %.3f secs\n", timeTaken.count()); 

        sTime = system_clock::now(); 
        start_simulator((char*)("./traces/" + kvtraces.first).c_str(), numtraces, EXCLUSIVE);
        eTime = system_clock::now();
        
        timeTaken = eTime-sTime;
        printf("Elapsed Time: %.3f secs\n", timeTaken.count()); 

        sTime = system_clock::now();         
        start_simulator((char*)("./traces/" + kvtraces.first).c_str(), numtraces, NINE);
        eTime = system_clock::now();

        timeTaken = eTime-sTime;
        printf("Elapsed Time: %.3f secs\n", timeTaken.count()); 
    }
    return 1;  
}