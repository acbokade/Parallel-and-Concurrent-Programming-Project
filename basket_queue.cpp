#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <ctime>
#include <math.h>
#include <unistd.h>
#include <random>
#include <vector>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <map>
#include <sstream>
#include <algorithm>
using namespace std;

mutex fileLock; // for output to log file as it is shared
// declaring pointer_t struct
struct pointer_t;

// declaring node_t struct
struct node_t;

// defining pointer_t struct
struct pointer_t {
    // node_t pointer ptr denoting the node being pointed
    node_t* ptr;
    // boolean deleted denoting whether node pointed by this pointer
    // is logically deleted or not
    bool deleted;
    // tag used to solve ABA problem
    int tag;

    pointer_t(node_t* node_ptr, bool given_deleted, int given_tag) noexcept: ptr(node_ptr), deleted(given_deleted), tag(given_tag) {}

    pointer_t() noexcept: ptr(NULL) {}

    // pointer_t& operator = (const pointer_t& other) {
    //     ptr = other.ptr;
    //     deleted = other.deleted;
    //     tag = other.tag;
    //     return *this;
    // }

    // defining equality operator
    bool operator == (const pointer_t& other) {
        return (this->ptr == other.ptr && this->deleted == other.deleted && this->tag == other.tag);
    }

    // defining not equality operator
    bool operator != (const pointer_t& other) {
        return !(*this == other);
    }
};

// defining node_t struct
struct node_t {
    // stores value of node
    int value;
    // stores pointer of next node 
    atomic<pointer_t> next;

    node_t() noexcept {}

    node_t(int val, pointer_t ptr) noexcept: value(val), next(ptr) {}

    node_t& operator = (const node_t& other) {
        value = other.value;
        next.store(other.next.load());
        return *this;
    }

    bool operator == (const node_t& other) {
        this->value == other.value && this->next.load() == other.next.load();
    }
};


class BasketQueue {
public:
    // MAX_HOPS denoting the maximum value of logically deleted nodes
    // to be present in queue before they are physically deleted i.e.
    // memory is reclaimed
    const int MAX_HOPS = 3;
    // const int MIN_DELAY = 1;
    // const int MAX_DELAY = 10;
    // tail pointer
    atomic<pointer_t> tail;
    // head pointer
    atomic<pointer_t> head;


    BasketQueue() {
        // defining dummy node
        node_t* new_node = new node_t();

        new_node->next.store({NULL, false, 0});

        // initially, both head and tail point to dummy node
        tail.store({new_node, false, 0});

        head.store({new_node, false, 0});
    }

    ~BasketQueue() {
        // freeing memory of all nodes present in queue finally
        if (this->head.load() != this->tail.load())
            free_chain(this->head.load(), this->tail.load());
    }

    // physically deletes all nodes from node pointed by head
    // till node pointed by new_head
    void free_chain(pointer_t head, pointer_t new_head) {
        if(this->head.compare_exchange_strong(head, pointer_t(new_head.ptr, 0, head.tag+1))) {
            pointer_t next;
            while(head.ptr != new_head.ptr) {
                next = head.ptr->next.load();
                delete head.ptr;
                head = next;
            }
        }
    }

    // enqueue operation with given val
    bool enqueue(int val) {
        // initialize new node to be inserted
        node_t* new_node = new node_t();
        // storing val as its value
        new_node->value = val;
        
        while(true) {
            // store tail of the queue at this point
            pointer_t local_tail = this->tail.load();
            // store next of tail of queue at this point
            pointer_t next = local_tail.ptr->next.load();

            // check if tail of queue and local tail are same
            if(local_tail == this->tail.load()) {
                if(next.ptr == NULL) {
                    new_node->next.store({NULL, 0, local_tail.tag+2});
                    // first thread in the basket
                    if(local_tail.ptr->next.compare_exchange_strong(next, pointer_t(new_node, 0, local_tail.tag+1))) {
                        this->tail.compare_exchange_strong(local_tail, pointer_t(new_node, 0, local_tail.tag+1));
                        return true;
                    }
                    // other threads of the same basket which failed CAS
                    next = local_tail.ptr->next.load();
                    while((next.tag == local_tail.tag+1) and !(next.deleted)) {
                        new_node->next = next;
                        if(local_tail.ptr->next.compare_exchange_strong(next, pointer_t(new_node, 0, local_tail.tag+1))) {
                            return true;
                        }
                        next = local_tail.ptr->next.load();
                    }
                } else {
                    // fixing tail of queue to point to last node of queue
                    while((next.ptr->next.load().ptr != NULL) and (this->tail.load() == local_tail)) {
                        next = next.ptr->next.load();
                    }
                    this->tail.compare_exchange_strong(local_tail, pointer_t(next.ptr, 0, local_tail.tag+1));
                }
            }
        }
    }

    int dequeue() {
        while(true) {
            pointer_t local_head = this->head.load();
            pointer_t local_tail = this->tail.load();
            pointer_t next = local_head.ptr->next.load();

            if(local_head == this->head.load()) {
                if(local_head.ptr == local_tail.ptr) {
                    if(next.ptr == NULL) 
                        return -1; // empty
                    // fixing tail pointer of queue to point to last node
                    while((next.ptr->next.load().ptr != NULL) and (this->tail.load() == local_tail)) {
                        next = next.ptr->next.load();
                    }
                    this->tail.compare_exchange_strong(local_tail, pointer_t(next.ptr, 0, local_tail.tag+1));
                }
                else {
                    pointer_t iter = local_head;
                    int hops = 0;
                    // skipping over logically deleted nodes
                    // next points to first unmarked node and iter points to last logically deleted node
                    while((next.deleted and iter.ptr != local_tail.ptr) and this->head.load() == local_head) {
                        iter = next;
                        next = iter.ptr->next.load();
                        hops++;
                    }
                    // head of queue has been changes thus continuing
                    if(this->head.load() != local_head) {
                        continue;
                    } else if(iter.ptr == local_tail.ptr) {
                        free_chain(local_head, iter);
                    } else {
                        int value = next.ptr->value;
                        // deleting node pointed by next and changing next of iter pointer
                        if(iter.ptr->next.compare_exchange_strong(next, pointer_t(next.ptr, 1, next.tag+1))) {
                            // if more than MAX_HOPS nodes are logically deleted, then physically deleting them
                            // reclaiming memory
                            if(hops >= MAX_HOPS) {
                                free_chain(local_head, next);
                            }
                            return value;
                        } 
                    }
                }
            }
        }
    }

};

// output file stream
ofstream output_file;

// number of threads and no of entries
int n_threads, n_entries;

default_random_engine generator;

// start time used to sort the write events
chrono::high_resolution_clock::time_point start;

// map containing writer thread events 
// key is time and value is long entry of type string
map<double, string> log_events;

// mutex lock for output file writing
mutex output_file_lock;

// 50% enqueue and 50% dequeue operations 
void testCS(int thread_id, BasketQueue* basket_queue) {
    // map with key as time of event log and value as
    // log output string 
    map<double, string> local_events;

    int n_enqueues = 1;
    int n_dequeues = 1;

    // uniform integer distribution used for uniformly
    // choosing enqueue or dequeue operation
    uniform_int_distribution<int> distribution(0, 1);
    int dequeued_number;
    
    for(int i=0;i<n_entries;i++) {
        int choice = distribution(generator);
        int rand_number = rand() % 100000000;

        // local log outputs string stream
        stringstream local_output_entry;
        stringstream local_output_exit;

        // recording time when thread requests enqueue or dequeue operation
        auto high_res_request_entry_time = chrono::high_resolution_clock::now();
        time_t request_entry_time_t = time(0);
        tm* request_entry_time = localtime(&request_entry_time_t);
        
        // storing log output in local buffer
        if(choice == 0) {
            fileLock.lock();
                output_file<<"Thr"<<thread_id<<"'s "<<n_enqueues<<"th enqueue request at "<<request_entry_time->tm_hour<<":"<<request_entry_time->tm_min<<":"<<request_entry_time->tm_sec<<"\n";
            fileLock.unlock();
            basket_queue->enqueue(rand_number);
        } else {
            fileLock.lock();
                output_file<<"Thr"<<thread_id<<"'s "<<n_dequeues<<"th dequeue request at "<<request_entry_time->tm_hour<<":"<<request_entry_time->tm_min<<":"<<request_entry_time->tm_sec<<"\n";
            fileLock.unlock();            
            dequeued_number = basket_queue->dequeue();
        }

        // recording time of completion of enqueue or dequeue operation
        auto high_res_exit_time = chrono::high_resolution_clock::now();
        time_t exit_time_t = time(0);
        tm* exit_time = localtime(&exit_time_t);

        // storing log output in local buffer
        if(choice == 0) {
            fileLock.lock();
                output_file<<"Thr"<<thread_id<<"'s "<<n_enqueues<<"th enqueue of "<<rand_number<<" completed at "<<exit_time->tm_hour<<":"<<exit_time->tm_min<<":"<<exit_time->tm_sec<<"\n";
            fileLock.unlock();
            n_enqueues++;
        } else {
            if(dequeued_number == -1){
                fileLock.lock();
                    output_file<<"Thr"<<thread_id<<"'s "<<n_dequeues<<"th dequeue reporting empty list completed at "<<exit_time->tm_hour<<":"<<exit_time->tm_min<<":"<<exit_time->tm_sec<<"\n";
                fileLock.unlock();
            }
            else {
                fileLock.lock();
                    output_file<<"Thr"<<thread_id<<"'s "<<n_dequeues<<"th dequeue of "<<dequeued_number<<" completed at "<<exit_time->tm_hour<<":"<<exit_time->tm_min<<":"<<exit_time->tm_sec<<"\n";
                fileLock.unlock();
            }
            n_dequeues++;
        }

    }    
}



int main() {
    // initialize random seed
    srand (time(NULL));
    
    // seed for default random engine generator
    // generator.seed(4);

    // input file stream
    ifstream input_file;
    input_file.open("inp-params.txt");

    // output file stream
    output_file.open("output.txt");

    // taking parameters from input file
    // n_threads denote number of threads
    // n_entries denote number of times each thread perform
    // enqueue or dequeue operation
    input_file>>n_threads>>n_entries;
    cout<<n_threads<<" "<<n_entries<<endl;

    // declaring threads
    thread worker_threads[n_threads];

    // initializating basket queue
    BasketQueue* basket_queue = new BasketQueue();

    // storing start time before all threads start
    // used to sort log events for printing
    start = chrono::high_resolution_clock::now();

    // assigning threads their task
    for(int i=0;i<n_threads;i++) {
        worker_threads[i] = thread(testCS, i, basket_queue);
    }

    // waiting for all threads to join
    for(int i=0;i<n_threads;i++)
        worker_threads[i].join();

    // write all log entries onto output file
    // Log entries are sorted by their time of entry

    basket_queue->free_chain(basket_queue->head.load(), basket_queue->tail.load());
    // reclaiming memory of log events

    // cleanup i.e. closing all the files
    input_file.close();
    output_file.close();
}