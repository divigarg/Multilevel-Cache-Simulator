#include <string>

#define INCLUSIVE 'i'
#define EXCLUSIVE 'e'
#define NINE      'n'

#define throw_error(err_msg, func)    \
({                               \
    char buf[512];                \
    sprintf(buf, err_msg, func);          \
    throw std::runtime_error(buf); \
})

#define is_null(addr) (addr == NULL)

typedef char policy;

struct env {
    
    std::string        trace;

    policy        cache_policy;

    unsigned long       l1_misses;
    unsigned long       l2_misses;
    unsigned long       l3_misses;
    unsigned long       l3_cold_misses;

    env(const std::string &t, policy p) {
        trace = t;
        cache_policy = p;
        l1_misses = 0;
        l2_misses = 0;
        l3_misses = 0;
        l3_cold_misses = 0;
    };

};

struct entry {

    char iord;
    char type;
    unsigned long long addr;
    unsigned pc;

};

void start_simulator(char*, int, policy);
void process_entry(struct entry*);
void print_stats();
void init_caches();