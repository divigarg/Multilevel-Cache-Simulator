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


void handle_signal(int s) {
    printf("Received signal %d\n", s);

    fflush(stdout);
    exit(1);
}


int main(int argc, char* argv[]) {

    struct args *_args = new struct args();

    argparse::ArgumentParser program("./bin/simulator");
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


    if(program["--belady"] == true){
        _args->belady = true;
    }
    


    printf("Running on filename: %s, num_traces: %d\n", \
            _args->filename, _args->num_traces);
        
    run(_args);

    free(_args);
    return 1;  
}