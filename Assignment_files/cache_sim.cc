#include <bits/stdc++.h>
using namespace std;

int L1_SIZE, L1_ASSOC, L1_BLOCKSIZE, VC_NUM_BLOCKS, L2_SIZE, L2_ASSOC;
string trace_file;


bool L2_PRESENT;


struct block{
    string tag;
    int set_index;
    string address;
    int dirty;
    block(string tag, int set_index, string address, int dirty){
        this->tag = tag;
        this->set_index = set_index;
        this->address = address;
        this->dirty = dirty;
    }
};

typedef struct block block;


int no_sets_util(int size, int assoc, int block_size){
    return ceil(1.0 * size / (assoc * block_size));
}

uint32_t hex_to_int(string hex_string){
    uint32_t num;
    std::stringstream ss;
    ss << std::hex << hex_string;
    ss >> num;
    return num;
}

string int_to_hex(uint32_t num){
    std::stringstream ss;
    ss << std::hex << num;
    string res = ss.str();
    return res;
}

block* decodeTagAndSetIndex(string hex_address, int block_size, int no_sets){
    int block_offset = log2(block_size);
    int set_offset = log2(no_sets);

    uint32_t address = hex_to_int(hex_address);
    uint32_t shifted = address >> block_offset;
    uint32_t mask = no_sets - 1;
    uint32_t set_index = shifted & mask;
    uint32_t tag = shifted >> set_offset;

    return (new block(int_to_hex(tag), set_index, int_to_hex(shifted), 0));
}

int find_address(vector<vector<block*>> &cache, string tag, int set_index){
    for(int i = 0; i < cache[set_index].size(); i++){
        if(cache[set_index][i]->tag == tag){
            return i;
        }
    }
    return -1;
}

void push_front(vector<block*> &cache_set, block* block){
    reverse(cache_set.begin(), cache_set.end());
    cache_set.push_back(block);
    reverse(cache_set.begin(), cache_set.end());
}

string reconstruct_address(string tag, int set_index, int block_size, int no_sets){
    uint32_t tag_int = hex_to_int(tag);
    uint32_t set_index_int = set_index;
    uint32_t address = (tag_int << (uint32_t)(log2(no_sets)) + set_index_int) << (uint32_t)log2(block_size);
    return int_to_hex(address);
}

block* reconstruct_block(block* old_block, int prev_no_sets, int new_no_sets){
    uint32_t address = hex_to_int(old_block->address);
    uint32_t tag = address >> (uint32_t)(log2(new_no_sets));
    uint32_t mask = new_no_sets - 1;
    uint32_t set_index = address & mask;
    return (new block(int_to_hex(tag), set_index, old_block->address, old_block->dirty));
}

void print_block(block* block){
    cout << block->tag << " " << block->set_index << " " << block->address << " " << block->dirty << endl;
}

class cache{

    public:

    int size;
    int assoc;
    int block_size;
    int no_sets;
    int reads;
    int writes;
    int read_misses;
    int write_misses;
    int write_backs;
    int swap_requests;
    int swaps_served;
    int vc_num_blocks;


    vector<vector<block*>> cache_array;

    cache(int size, int assoc, int block_size){
        this->size = size;
        this->assoc = assoc;
        this->block_size = block_size;
        this->no_sets = no_sets_util(size, assoc, block_size);
        this->reads = 0;
        this->writes = 0;
        this->read_misses = 0;
        this->write_misses = 0;
        this->write_backs = 0;
        this->swap_requests = 0;
        this->swaps_served = 0;
        this->vc_num_blocks = 0;

        this->cache_array = vector<vector<block*>>(no_sets, vector<block*>());
    }
    

    block* perform_operation(string address, cache* victim_cache, int is_write){
        is_write ? writes++ : reads++;
        block* new_block = decodeTagAndSetIndex(address, block_size, no_sets);
        string tag = new_block->tag;
        int set_index = new_block->set_index;
        new_block->dirty = is_write;
        
        int set_number = find_address(cache_array, tag, set_index);

        if(set_number != -1){
            // Block was found in cache

            rotate(cache_array[set_index].begin(), cache_array[set_index].begin() + set_number, 
                cache_array[set_index].begin() + set_number + 1);
            cache_array[set_index][0]->dirty |= is_write;
            return (new block("", -1, "", -1));
        }
        else{
            // Block was not found in cache

            is_write ? write_misses++ : read_misses++;
            
            if(cache_array[set_index].size() < assoc){
                // Don't need to evict any block from current level
                // Have to perform read from next level
                push_front(cache_array[set_index], new_block);
                return (new block("", -1, "", -2));
            } else {
                // Need to evict a block from current level
                // Have to write back to next level if evicted block is dirty
                

                block* evicted_block = reconstruct_block((cache_array[set_index]).back(), no_sets, 1);
                assert(((cache_array[set_index]).back())->address == evicted_block->address);
                assert(evicted_block->tag == evicted_block->address);
                assert(evicted_block->set_index == 0);
                assert(evicted_block->dirty == 0 || evicted_block->dirty == 1);
                assert(evicted_block->dirty == ((cache_array[set_index]).back())->dirty);
                (cache_array[set_index]).pop_back();
                // cout<<"search ("<<tag<<":"<<new_block->address<<") == ";
                // assert(cache_array[set_index].size() == assoc);
                if(vc_num_blocks != 0){
                    // VC enabled for current cache level
                    swap_requests++;
                    // push_front(cache_array[set_index], new_block);
                    
                    int vc_set_index = find_address(victim_cache->cache_array, new_block->address, 0);
                    if(vc_set_index != -1){
                        // Block found in VC
                        // Swap evicted block with block in VC
                        // No need to read from next level

                        // cout<<"swap ==> ("<<evicted_block->tag<<":"<<evicted_block->address<<")"<<endl;
                        swaps_served++;
                        // cache_array[set_index][0]->dirty |= ((victim_cache->cache_array)[0][vc_set_index])->dirty;
                        new_block->dirty |= ((victim_cache->cache_array)[0][vc_set_index])->dirty;
                        (victim_cache->cache_array)[0][vc_set_index] = evicted_block;
                        rotate(victim_cache->cache_array[0].begin(), victim_cache->cache_array[0].begin() + vc_set_index, 
                            victim_cache->cache_array[0].begin() + vc_set_index + 1);
                        push_front(cache_array[set_index], new_block);
                        // cache_array[set_index].push_back(new_block);

                        return (new block("", -1, "", -1));
                    }
                    else{
                        // Block not found in VC
                        // Have to read from next level
                        int old_size = (victim_cache->cache_array)[0].size();
                        if((victim_cache->cache_array)[0].size() < vc_num_blocks){
                            // VC has space for evicted block; just add to front

                            push_front((victim_cache->cache_array)[0], evicted_block);
                            assert((victim_cache->cache_array)[0].size() == (old_size + 1));
                            push_front(cache_array[set_index], new_block);

       
                            return (new block("", -1, "", -2));
                        } else {
                            // VC has no space for evicted block
                            // Evict LRU block from VC and add evicted block to front

                            block* evicted_vc_block = (victim_cache->cache_array)[0].back();
                            (victim_cache->cache_array)[0].pop_back();
                            push_front((victim_cache->cache_array)[0], evicted_block);
                            assert(victim_cache->cache_array[0].size() == vc_num_blocks);
                            assert(evicted_block->tag == evicted_block->address);
                            assert(evicted_vc_block->dirty == 0 || evicted_vc_block->dirty == 1);
                            if(evicted_vc_block->dirty == 1){
                                write_backs++;
                            }
                            push_front(cache_array[set_index], new_block);

                            return evicted_vc_block;
                        }
                    }
                }
                assert(evicted_block->tag == evicted_block->address);
                assert(evicted_block->dirty == 0 || evicted_block->dirty == 1);
                if(evicted_block->dirty == 1){
                    write_backs++;
                }
                push_front(cache_array[set_index], new_block);
                return evicted_block;
            }
        }

    }


};

int main(int argc, char** argv) {


    if (argc != 8){
        cout << "Usage: ./cache_sim <L1_SIZE> <L1_ASSOC>\n"
                "                   <L1_BLOCKSIZE> <VC_NUM_BLOCKS>\n"
                "                   <L2_SIZE> <L2_ASSOC>\n"
                "                   <trace_file>" << endl;
        return 0;
    }

    L1_SIZE = atoi(argv[1]);
    L1_ASSOC = atoi(argv[2]);
    L1_BLOCKSIZE = atoi(argv[3]);
    VC_NUM_BLOCKS = atoi(argv[4]);
    L2_SIZE = atoi(argv[5]);
    L2_ASSOC = atoi(argv[6]);
    trace_file = argv[7];
    L2_PRESENT = (L2_SIZE != 0);
    

    cout << "===== Simulator configuration =====\n"
            "  L1_SIZE: \t" << L1_SIZE << "\n"
            "  L1_ASSOC: \t" << L1_ASSOC << "\n"
            "  L1_BLOCKSIZE: \t" << L1_BLOCKSIZE << "\n"
            "  VC_NUM_BLOCKS: \t" << VC_NUM_BLOCKS << "\n"
            "  L2_SIZE:	\t" << L2_SIZE << "\n"
            "  L2_ASSOC: \t" << L2_ASSOC << "\n"
            "  trace_file: \t" << trace_file << "\n" << endl;


    cache* L1_CACHE = new cache(L1_SIZE, L1_ASSOC, L1_BLOCKSIZE);
    L1_CACHE->vc_num_blocks = VC_NUM_BLOCKS;


    cache* victim_cache = NULL;
    if(VC_NUM_BLOCKS != 0){
        victim_cache = new cache(L1_BLOCKSIZE*VC_NUM_BLOCKS, VC_NUM_BLOCKS, L1_BLOCKSIZE);
    }

    cache* L2_CACHE = NULL;
    if(L2_PRESENT){
        L2_CACHE = new cache(L2_SIZE, L2_ASSOC, L1_BLOCKSIZE);
    }
    // cout << L1_CACHE->size() << " sadf " << L1_CACHE[0].size() << endl;

    ifstream traceFile(trace_file); 
  
    if (!traceFile.is_open()) { 
        cerr << "Error opening the trace file!" << endl; 
        return 1; 
    } 
  
    string line; 
  
    while (getline(traceFile, line)) { 

        istringstream iss(line);

        string access_type;
        string address;
        
        iss >> access_type >> address;

        if(address.size() < 8){
            address = string(8 - address.size(), '0') + address;
        }

        // if(decodeTagAndSetIndex(address, L1_BLOCKSIZE, L1_CACHE->no_sets)->set_index == 4){
        //     cout<<access_type<<" "<<decodeTagAndSetIndex(address, L1_BLOCKSIZE, L1_CACHE->no_sets)->tag<<endl;
        // }
        // cout<<access_type<<" "<<decodeTagAndSetIndex(address, L1_BLOCKSIZE, L1_CACHE->no_sets)->tag<<endl;

        // cout<<(access_type == "W")<<endl;
        block* evicted_block = L1_CACHE->perform_operation(address, victim_cache, access_type == "w");
        
        if(evicted_block->dirty == -1){
            continue;
        } else{
            if(L2_PRESENT){
                if(evicted_block->dirty == 1){
                    string recon_address = int_to_hex(hex_to_int(evicted_block->address)*L1_BLOCKSIZE);
                    L2_CACHE->perform_operation(recon_address, NULL, 1);
                }
                L2_CACHE->perform_operation(address, NULL, 0);
            }
        } 

    }

    cout << "===== L1 contents =====" << endl;
    for(int i = 0; i < L1_CACHE->no_sets; i++){
        cout << "Set\t" << i << ":";
        for(int j = 0; j < L1_CACHE->cache_array[i].size(); j++){
            cout << " " << L1_CACHE->cache_array[i][j]->tag;
            if(L1_CACHE->cache_array[i][j]->dirty == 1){
                cout << " D";
            }
            else{
                cout << "  ";
            }
        }
        cout << endl;
    } 
    cout << endl;

    if(VC_NUM_BLOCKS != 0){
        cout << "===== VC contents =====" << endl;
        for(int i = 0; i < victim_cache->no_sets; i++){
            cout << "Set\t" << i << ":";
            for(int j = 0; j < victim_cache->cache_array[i].size(); j++){
                cout << " " << victim_cache->cache_array[i][j]->tag;
                if((victim_cache->cache_array[i][j])->dirty == 1){
                    cout << " D";
                }
                else{
                    cout << "  ";
                }
            }
            cout << endl;
        } 
        cout << endl;
    }

    if(L2_PRESENT){
        cout << "===== L2 contents =====" << endl;
        for(int i = 0; i < L2_CACHE->no_sets; i++){
            cout << "Set\t" << i << ":";
            for(int j = 0; j < L2_CACHE->cache_array[i].size(); j++){
                cout << " " << L2_CACHE->cache_array[i][j]->tag;
                if(L2_CACHE->cache_array[i][j]->dirty == 1){
                    cout << " D";
                }
                else{
                    cout << "  ";
                }
            }
            cout << endl;
        } 
        cout << endl;
    }


    cout << "===== Simulation results (raw) =====" << endl;
    cout << "a. number of L1 reads: \t" << L1_CACHE->reads << endl;
    cout << "b. number of L1 read misses: \t" << L1_CACHE->read_misses << endl;
    cout << "c. number of L1 writes: \t" << L1_CACHE->writes << endl;
    cout << "d. number of L1 write misses: \t" << L1_CACHE->write_misses << endl;
    cout << "e. number of swap requests: \t" << L1_CACHE->swap_requests << endl;
    cout << "f. swap request rate: \t" << 0.0000 << endl;
    cout << "g. number of swaps: \t" << L1_CACHE->swaps_served << endl;
    cout << "h. combined L1+VC miss rate: \t" << (1.0 * (L1_CACHE->read_misses + L1_CACHE->write_misses - L1_CACHE->swaps_served) / (L1_CACHE->reads + L1_CACHE->writes)) << endl;
    cout << "i. number writebacks from L1/VC: \t" << L1_CACHE->write_backs << endl;
    if(L2_PRESENT) cout << "j. number of L2 reads: \t" << L2_CACHE->reads << endl;
    else cout << "j. number of L2 reads: \t" << 0 << endl;
    if(L2_PRESENT) cout << "k. number of L2 read misses: \t" << L2_CACHE->read_misses << endl;
    else cout << "k. number of L2 read misses: \t" << 0 << endl;
    if(L2_PRESENT) cout << "l. number of L2 writes: \t" << L2_CACHE->writes << endl;
    else cout << "l. number of L2 writes: \t" << 0 << endl;
    if(L2_PRESENT) cout << "m. number of L2 write misses: \t" << L2_CACHE->write_misses << endl;
    else cout << "m. number of L2 write misses: \t" << 0 << endl;
    if(L2_PRESENT) cout << "n. L2 miss rate: \t" << (1.0 * (L2_CACHE->read_misses) / (L2_CACHE->reads)) << endl;
    else cout << "n. L2 miss rate: \t" << 0.0000 << endl;
    if(L2_PRESENT) cout << "o. number of writebacks from L2: \t" << L2_CACHE->write_backs << endl;
    else cout << "o. number of writebacks from L2: \t" << 0 << endl;
    // cout << "p. total memory traffic: \t" << L1_CACHE->read_misses + L1_CACHE->write_misses + L1_CACHE->write_backs + L2_READ_MISSES + L2_WRITE_MISSES + L2_WRITE_BACKS << endl;


}
