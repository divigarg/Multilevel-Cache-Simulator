#pragma once

#include <iostream>
#include <map>
#include <simulator.h>
#include <signal.h>
#include <string>
#include <typeinfo>

#include <argparse/argparse.hpp>

using namespace std;
using namespace std::chrono;


FILE *_debug;

void handle_signal(int s) {
    printf("Received signal %d\n", s);

    fflush(_debug);
    exit(1);
}


int main(int argc, char* argv[]) {

    struct args *_args = new struct args();

    argparse::ArgumentParser program("./bin/simulator");
    // if(argc == 1) cout << "Format: ./bin/simulator <./traces/---.log_l1misstrace> <numTraces> <./logs/debug.log> [<max_alloc>] [<if max_alloc, belady>]" << endl;
    for (int i =0; i < 19; i++)
        signal(i, handle_signal);

    program.add_argument("--file")
        .required()
        .help("run simulator on given trace file");

    program.add_argument("num_traces")
        .help("number of parts of given trace file")
        .scan<'i', int>();

    
    program.add_argument("--max_assoc")
        .help("use fully associative cache")
        .default_value(false)
        .implicit_value(true);
    
    program.add_argument("--output")
        .required()
        .help("output log file");

    program.add_argument("--belady")
        .help("use belady replacement policy  (requires --max_assoc flag)")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }


    _args->filename = (char*)(program.get<std::string>("--file").c_str());


    _args->num_traces = program.get<int>("num_traces");

    if (program["--max_assoc"] == true) {
        _args->full_assoc = true;
    }

    _args->log_file = (char*)(program.get<std::string>("--output").c_str());

    if(program["--belady"] == true){
        _args->belady = true;
    }
    



    _debug = fopen(_args->log_file, "w");

    printf("filename: %s, num_traces: %d, log_file: %s\n", \
            _args->filename, _args->num_traces, _args->log_file);
        
    run(_args);

    free(_args);
    return 1;  
}