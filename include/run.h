#include <string>

#define INCLUSIVE 'i'
#define EXCLUSIVE 'e'
#define NINE      'n'

#define LOCK    pthread_mutex_lock(&_lock);
#define UNLOCK  pthread_mutex_unlock(&_lock);

#define throw_error(err_msg, func)    \
({                               \
    char buf[512];                \
    sprintf(buf, err_msg, func);          \
    throw std::runtime_error(buf); \
})

#define is_null(addr) (addr == 0)

typedef char policy;

// struct env {
    
//     std::string        trace;

//     policy        cache_policy;

//     unsigned long       l1_misses;
//     unsigned long       l2_misses;
//     unsigned long       l3_misses;

//     env(const std::string &t, policy p) {
//         trace = t;
//         cache_policy = p;
//         l1_misses = 0;
//         l2_misses = 0;
//         l3_misses = 0;
//     };

// };

struct args {
    char* filename;
    int num_traces;
    bool full_assoc;
    char* log_file;
    bool belady;
    policy _policy;
    
    args() {
        filename = {0};
        num_traces = 0;
        full_assoc = false;
        log_file = {0};
        belady = false;
    };

};

struct entry {

    char iord;
    char type;
    unsigned long long addr;
    unsigned pc;

    int counter;
};

void run(struct args*);