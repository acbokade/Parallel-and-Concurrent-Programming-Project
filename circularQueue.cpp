#include<bits/stdc++.h>
#include <sys/time.h>
#include <unistd.h>
using namespace std;
#define ff first
#define ss second
map <pair<long long,int> , pair<string,pair<int,int>>> logs; // Stores the logs to output to Log file
int k,l1,l2; 
std::default_random_engine eng;
std::default_random_engine generator;
mutex fileLock; // for output to log file as it is shared
ofstream output;  // output stream
class Helper { 
    public:
    static string get_formatted_time(time_t t1) // gives formatted time
    {
        struct tm* t2=localtime(&t1);
        char buffer[20];
        sprintf(buffer,"%d : %d : %d",t2->tm_hour,t2->tm_min,t2->tm_sec);
        return buffer;
    }
};
struct entry{ // entry of queue is a pair
	int cycle,index;
};
 // atomicEntry is an atomic wrapper of entry so that we can copy , assign entry atomically
class atomicEntry{
    public:
        atomic < entry > c;
    atomicEntry(){
        c.store({0,0});
    }
    atomicEntry(int cycle,int index){
        c.store({cycle,index});
    }
    atomicEntry (const atomicEntry &other){
        c.store(other.c.load()); // store to write to a register , load to read the register
    }
    atomicEntry &operator=(const  atomicEntry &other){
        c.store(other.c.load());
    }   
};
/**
 * Queue class
 * capacity - max number of items in queue
 * atomic <int> tail , head - atomic variables denoting tail and head of list
 * atomicEntry* entries - array to store entries of queue
 * enqueue(index) method add index to the queue
 * dequeue() method dequeues an element from the queue
 */
class Queue{
	private:
	int capacity;
	atomic<int> tail, head;
	atomicEntry * entries ;
	public: Queue(int flag,int capacity){
		this->capacity = capacity;
		entries = new atomicEntry[capacity];
        // Empty queue is initialized with tail = head = n
		tail.store(capacity,memory_order_relaxed); 
		head.store(capacity,memory_order_relaxed);
		if(flag == 1){ // for full queue we enqueue items from 1 to n 
			for(int i=0;i<capacity;i++){
				this->enqueue(i);			
			}
		}
	}

	timeval enqueue(int index){
		int j,t;
		entry ent,new1;
		do{
			label:
			t = tail.load(); 
			j = (t) % capacity;
			ent = entries[j].c.load(); 
			if(ent.cycle == (t/capacity)){ // check if cycle of entry is same as cycle of tail
				tail.compare_exchange_strong(t,t+1);
				goto label;
			}

			if((ent.cycle + 1) != (t/capacity))
				goto label;
			
			new1 = {t/capacity , index}; // cycle of entry is same as cycle of tail
		}while(!entries[j].c.compare_exchange_strong(ent,new1)); // try to update entries[j] with new value
		struct timeval t_req;
		gettimeofday(&t_req,NULL); // Linearization point successfull CAS
		tail.compare_exchange_strong(t,t+1); // update tail
		return t_req;
	}
	pair<int,timeval> dequeue(){
		
		int h,j;
		entry ent;
		do{
			label2:
			h = head.load();
			j = h % capacity;
			ent = entries[j].c.load();
			if((ent.cycle) != (h/capacity)){
				if ((ent.cycle + 1) == (h/capacity) )
					return {-1,timeval()};
				goto label2;
			}
            // cycle of head and entry are same => we can dequeue
		}while(!head.compare_exchange_strong(h,h+1));
		struct timeval t_req;
		gettimeofday(&t_req,NULL);// Linearization Point successful CAS
		return {ent.index,t_req}; // return index and time
	}
};
/**
 * The data entries are not stored in the queue itself, 
 * insted a queue entry records an index in the array of data.
 * For simplicity we consider the data to be integers * 
 * fq - It keeps the indices to unallocated entries of the data array. 
 * aq - This queue keeps allocated indices which are to be consumed.    
 * 
 */
class CircularQueue {
    int* data; // data array 
    Queue *aq, *fq; // 2 queues which will store the indices , from index we retrieve data
	int capacity; // capacity 
    public:
    CircularQueue(int capacity){
		this->capacity = capacity;
		data = new int[capacity];
		aq = new Queue(0,capacity);
		fq = new Queue(1,capacity);
    }

	pair<int,timeval> enque_data(int val){
		pair<int,timeval> enqueIndex = fq->dequeue(); // first enqueue from fq
		if(enqueIndex.first == -1) return {0,timeval()}; // Not equal to -1 means queue is not full
		data[enqueIndex.first] = val ; // update data array
		timeval enqueueTime = aq->enqueue(enqueIndex.first);  // add this index to aq 
		return {1,enqueueTime};
	}

	pair<int,timeval> dequeue_data(){
		pair<int,timeval> dequeIndex = aq->dequeue(); // first dequeue from aq
		if(dequeIndex.first == -1) return {-1,timeval()}; // Not equal to -1 means queue is not full
		int val = data[dequeIndex.first];  // get the value 
		timeval enqueueTime= fq->enqueue(dequeIndex.first); // enqueue in to fq 
		return {val,enqueueTime};
	}
};
struct threadData{ 
	CircularQueue* circularQueue;
    int id;
};	

void* testCS(void* args){
	uniform_int_distribution<int> distribution(0, 1);
	struct threadData* tdata=(struct threadData*)(args);
    int id = tdata->id; // extract data from passed arguments
	CircularQueue* circularQueue = tdata->circularQueue;
	std::exponential_distribution<double>exponential2(1.0);
	for(int i=0;i<k;i++)
	{
		int choice = distribution(generator);
		struct timeval t_req,t_enter,t_exit,t_exitReq;
		int val = rand() % 100000000 ;	
		
	
		time_t Request_time=time(NULL);
		string formatted_time=Helper::get_formatted_time(Request_time);       
		if(choice ==0 ){
			string FinalString1 = "Thr"+to_string(id+1) + " reqested enqueue of"+ to_string(val) + " for the "+ to_string(i) +  "th time on "+ to_string(t_req.tv_sec) + " "+ to_string(t_req.tv_usec) + "\n";
            fileLock.lock();
			output<<FinalString1;
            fileLock.unlock();
			pair<int,timeval> successfull = circularQueue->enque_data(val);
			string FinalString2 = "Thr"+to_string(id+1) + " successfully enqueued "+ to_string(val) + " for the "+ to_string(i) +  "th time on "+ to_string(successfull.second.tv_sec) + " "+ to_string(successfull.second.tv_usec) + "\n";
			if(successfull.first == 1){
				fileLock.lock();
				output<<FinalString2;
                fileLock.unlock();
			}
		}

		else{
			string FinalString1 = "Thr"+to_string(id+1) + " reqested dequeue for the "+ to_string(i) +  "th time on "+ to_string(t_req.tv_sec) + " "+ to_string(t_req.tv_usec) + "\n";
			fileLock.lock();
            output<<FinalString1;   
            fileLock.unlock();
			pair<int,timeval> val = circularQueue->dequeue_data();
			string FinalString = "Thr"+to_string(id+1) + " successfully dequeued "+ to_string(val.first) + " for the "+ to_string(i) + "th time on "+ to_string(val.second.tv_sec) + " "+ to_string(val.second.tv_usec) + "\n";
			if(val.first != -1){
				fileLock.lock();
				output<<FinalString;
                fileLock.unlock();
			}			
		}
	}
}
int main(){
	srand(time(NULL));
    int n;
    ifstream input("inp-params.txt"); // take input from inp-params.txt
	input>>n>>k>>l1>>l2;
    output.open("output.txt");
	pthread_t tid[n+1];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
	CircularQueue* circularQueue = new CircularQueue(10000000);
    for(int i=0;i<n;i++) // create n threads
    {
        struct threadData* tdata = new threadData{circularQueue,i};            
      	pthread_create(&tid[i],&attr,testCS,(void*)(tdata));		
    }

    for(int i=0;i<n;i++) // join threads
    {
        pthread_join(tid[i],NULL);
    }


	return 0;   
    
}
