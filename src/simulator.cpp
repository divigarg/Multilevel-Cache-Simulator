#include <cache.h>
#include <simulator.h>
#include <stdlib.h>
#include <iostream>
#include <bits/stdc++.h>
using namespace std;


extern pthread_mutex_t _lock;

void simulator::preprocess_belady(const char* filename, int numtraces) {
    LOCK printf("%s: %c -> processing input data for belady\n", __func__, cache_policy); UNLOCK
    int belady_index = 0;
    
    FILE *fp;
    char input_name[256];
    struct entry *_entry = new struct entry;

    for(int k=0; k < numtraces; k++){
        sprintf(input_name, "%s_%d", filename, k);
        // fprintf(_debug,"inputname: %s\n", input_name);
        fp = fopen(input_name, "rb");

        assert(fp != NULL);

        while (!feof(fp)) {
            fread(&_entry->iord, sizeof(char), 1, fp);
            fread(&_entry->type, sizeof(char), 1, fp);
            fread(&_entry->addr, sizeof(unsigned long long), 1, fp);
            fread(&_entry->pc, sizeof(unsigned), 1, fp);

            if(_entry->type){
                unsigned long long tmp_addr = _entry->addr;
                tmp_addr >>= l3_cache->block_bits;
                l3_cache->prebeladyData[tmp_addr].first = 0;
                l3_cache->prebeladyData[tmp_addr].second.emplace_back(belady_index);
                belady_index++;
            } 
        }
        
        fclose(fp);
    }

    for(auto& kv: l3_cache->prebeladyData){
        l3_cache->belady_sort.insert(make_pair(kv.second.second[0], kv.first));
    }

    LOCK printf("%s: %c -> processed input data for belady\n", __func__, cache_policy); UNLOCK
}


void simulator::start_simulator(const char* filename, int numtraces, bool belady) {

    // LOCK printf("%s: %c -> starting simulator\n", __func__, cache_policy); UNLOCK
    char* tmpfilename = (char *) malloc(strlen(filename));
    strcpy(tmpfilename, filename);
    char *token = strtok(tmpfilename, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, "/");
    token = strtok(token, ".");
    
    FILE *fp;
    char input_name[256];

    struct entry *_entry = (struct entry*)malloc(sizeof(struct entry));
    ///

    if(belady) this->preprocess_belady(filename, numtraces);

    int entry_index = 0;
    for (int k=0; k<numtraces; k++) {
        sprintf(input_name, "%s_%d", filename, k);
        // fprintf(_debug,"inputname: %s\n", input_name);
        fp = fopen(input_name, "rb");
        
        assert(fp != NULL);

        while (!feof(fp)) {
            fread(&_entry->iord, sizeof(char), 1, fp);
            fread(&_entry->type, sizeof(char), 1, fp);
            fread(&_entry->addr, sizeof(unsigned long long), 1, fp);
            fread(&_entry->pc, sizeof(unsigned), 1, fp);
            // LOCK
            // printf("%s: Processing addr: %p type: %c\n",__func__, _entry->addr, _entry->type);
            // UNLOCK;
            _entry->counter = entry_index;
            if(_entry->type) entry_index++;
            process_entry(_entry);
            // Process the entry
        }
        fclose(fp);
        // fprintf(_debug,"Done reading file %d!\n", k);
    }

    free(_entry);


}

void simulator::init_caches(bool assoc, bool belady) {

    l2_cache = new Cache (8, 64, 512 KB, L2);

    if (assoc) {
        policy rpl_policy;
        if(belady) rpl_policy = BELADY;
        else rpl_policy = LRU;
    
        l3_cache = new Cache (32768, 64, 2 MB, L3, rpl_policy);
    }
    else l3_cache = new Cache (16, 64, 2 MB, L3);


}

void simulator::process_entry(struct entry *_entry) {
    
    /*
        Process the entry based on memory hierarchy defined
        L1 -> L2 -> L3 -> Main Memory
        
    */

    if (lookup_l1(_entry))
        return;

    unsigned long long addr = 0;

    unsigned long long shift_addr = _entry->addr >> l3_cache->block_bits;

    if (l3_cache->repl_policy == BELADY){
        l3_cache->prebeladyData[shift_addr].first++;
        //belady optimization
        // auto &tmpDataPair = l3_cache->prebeladyData[shift_addr];
        // l3_cache->belady_sort.erase(make_pair(tmpDataPair.second[tmpDataPair.first-1], shift_addr));
        // l3_cache->belady_sort.insert(make_pair(tmpDataPair.second[tmpDataPair.first], shift_addr));
    }

    l1_misses++;

    // get block for L2 lookup
    l2_cache->get_block(_entry->addr, l2_block);

    l2_cache->lookup(l2_block);
    if (l2_block->valid) {
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
            
        goto clean_l2;
    }

    l2_misses++;

    l3_cache->get_block(_entry->addr, l3_block);
    l3_cache->lookup(l3_block);
    
    if (l3_block->valid) {
        
        /*
        EXCLUSIVE: L2-M-L3-H
        * Invalidate the target block in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, put it in L3
        * If a block gets evicted from L3, enough of this now :(
        */
        if (cache_policy == EXCLUSIVE) {
            l3_cache->invalidate(l3_block);
                
            l2_cache->copy(l2_block); // L2 block is not valid here -> Wont be a problem though
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
            
            if (l2_cache->victim != NULL){
                convert_l2_to_l3(l2_cache->victim, _victim);
                // l3_block->valid = true;
                //change made from l3_block to _victim since the victim needs to be added to l3
                l3_cache->copy(_victim);
                l3_cache->update_repl_params(_victim->index, _victim->way);
            }

        }

        /*
        INCLUSIVE: L2-M-L3-H
        * Update the LRU replacement params for target index in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, let it suffer :)
        */
        else if (cache_policy == INCLUSIVE) {

            l3_cache->update_repl_params(l3_block->index, l3_block->way);
            l2_cache->copy(l2_block);
            l2_cache->update_repl_params(l2_block->index, l2_block->way);
        }

        /*
        NINE: L2-M-L3-H
        * Update the LRU replacement params for target index in L3
        * Copy the target block back to L2
        * If a block gets evicted from L2, I dont care :)
        */
        else { // NINE Policy

            l3_cache->update_repl_params(l3_block->index, l3_block->way);
            l2_cache->copy(l2_block);
            l2_cache->update_repl_params(l2_block->index, l2_block->way);

        }

        goto clean_l3;
    }

    l3_misses++;

    
    if(fully_assoc)
        if(miss_already.find(_entry->addr >> l3_cache->block_bits) == miss_already.end()){
            cold_misses++;
            miss_already.insert(_entry->addr >> l3_cache->block_bits);
        }
    
    // Handle L3 miss
    // After fetching block from main memory

    /*
    EXCLUSIVE: L2-M-L3-M
    * Put the target block in L2
    * If a block gets evicted from L2, put it in L3
    * If a block gets evicted from L3, no problem !
    */
    if (cache_policy == EXCLUSIVE) {
        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);

        /*
        Check if any block got replaced in L2 cache
        */
        if (l2_cache->victim != NULL) {
            convert_l2_to_l3(l2_cache->victim, _victim);
            l3_cache->copy(_victim);
            l3_cache->update_repl_params(_victim->index, _victim->way);
        }
    }

    /*
    INCLUSIVE: L2-M-L3-M
    * Put the target block in L3 first
    * If the block gets evicted from L3, invalidate it from L2 if present
    * Put the target block in L2 now
    * If a block get evicted from L2, relax!
    */
    else if (cache_policy == INCLUSIVE) {

        l3_cache->copy(l3_block, _entry->counter);
        l3_cache->update_repl_params(l3_block->index, l3_block->way);
        /*
            Check if any block got evicted from L3, invalidate it from L2 also
            Don't act stupid, you are being watched!
        */
        if (l3_cache->victim != NULL) {

            convert_l3_to_l2(l3_cache->victim, _victim);
   
            l2_cache->lookup(_victim);
            
            if (_victim->valid) {
                l2_cache->invalidate(_victim);
            }
            
            
        }

        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);
        
    }

    /*
    NINE: L2-M-L3-M
    * Put the target block in L3 first
    * If a block gets evicted from L3, chill 
    * Put the target block in L2 now
    * If a block gets evicted from L2, put it in L3
    */
    else {
        l3_cache->copy(l3_block);
        l3_cache->update_repl_params(l3_block->index, l3_block->way);

        l2_cache->copy(l2_block);
        l2_cache->update_repl_params(l2_block->index, l2_block->way);

    }


clean_l3:
    l3_block->index = -1;
    l3_block->tag = -1;
    l3_block->valid = false;
    l3_block->way = -1;

    _victim->index = -1;
    _victim->tag = 0;
    _victim->valid = false;
    _victim->way = -1;

    if (!is_null(l2_cache->victim)) free(l2_cache->victim);
    if (!is_null(l3_cache->victim)) free(l3_cache->victim);
    
clean_l2:
    l2_block->index = -1;
    l2_block->tag = 0;
    l2_block->valid = false;
    l2_block->way = -1;
    l2_cache->victim = NULL;
    l3_cache->victim = NULL;
  

    return;
}

void simulator::print_stats() {
    cout << "L1 Misses " << l1_misses << endl;
    cout << "L2 Misses " << l2_misses << endl;
    cout << "L3 Misses " << l3_misses << endl;
    if(fully_assoc){
        cout << "L3 Cold Misses " << cold_misses << endl;
        cout << "L3 Capacity Misses " << l3_misses - cold_misses << endl;
    }
}

void simulator::clean_memory() {
    
    free(l2_cache);
    free(l3_cache);
    free(l2_block);
    free(l3_block);
    free(_victim);
}