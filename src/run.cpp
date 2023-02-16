#pragma once

#include <cache.h>
#include <simulator.h>
#include <stdlib.h>
#include <iostream>
#include <bits/stdc++.h>
#include <chrono>



using namespace std;

pthread_mutex_t _lock;



map<policy, string> policyString = {{INCLUSIVE, "Inclusive"}, {EXCLUSIVE, "Exclusive"}, {NINE, "Nine"}};


void *run_thread(void* _args) {

    struct args *__args = (struct args*)_args;
    simulator *_simulator = new simulator(__args->_policy, __args->full_assoc);
    _simulator->init_caches(__args->full_assoc, __args->belady);
    chrono::time_point<chrono::system_clock> sTime, eTime;
    
    sTime = chrono::system_clock::now(); 
    _simulator->start_simulator(__args->filename, __args->num_traces, __args->belady);
    eTime = chrono::system_clock::now();

    _simulator->clean_memory();
    chrono::duration<double> timeTaken = eTime-sTime;

    LOCK

    printf("----------------STATS--------------------\n");
    _simulator->print_stats();
    printf("%s: Elapsed Time: %.3f secs\n", policyString[__args->_policy].c_str(), timeTaken.count());
    printf("------------------------------------\n\n");

    UNLOCK;

    return NULL;
}

void run(struct args *_args) {
    // create three threads
    pthread_t inclusive_t, exclusive_t, nine_t;
    struct args *_inc_args, *_exc_args, *_nine_args;

    if (pthread_mutex_init(&_lock, NULL)) {
        printf("mutex init failed\n");
        exit (1);
    }

    _inc_args = new struct args();

    
    memcpy(_inc_args, _args, sizeof(struct args));


    // printf("%s:creating threads\n", __func__);

    
    _inc_args->_policy = INCLUSIVE;
    
    pthread_create(&inclusive_t, NULL, run_thread, (void*)_inc_args);

    if (_args->full_assoc)
        goto exit_run;
    

    _exc_args = new struct args();
    _nine_args = new struct args();

    memcpy(_exc_args, _args, sizeof(struct args));
    memcpy(_nine_args, _args, sizeof(struct args));

    _nine_args->_policy = NINE;
    _exc_args->_policy = EXCLUSIVE;

    pthread_create(&exclusive_t, NULL, run_thread, (void*)_exc_args);

    pthread_create(&nine_t, NULL, run_thread, (void*)_nine_args);
    pthread_join(exclusive_t, NULL);
    pthread_join(nine_t, NULL);

    free(_exc_args);
    free(_nine_args);
    
exit_run:
    pthread_join(inclusive_t, NULL);
    free(_inc_args);
    pthread_mutex_destroy(&_lock);    
}