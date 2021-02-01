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
class Queue{
    private:
    int capacity;
    int head , tail ;
    int* entries;
    mutex lock1;
    public: Queue(int capacity){
		this->capacity = capacity;
        entries = new int[capacity];
        head = tail = 0;
    }
    bool enqueue(int data){
       lock1.lock();
       if ((tail - head) == capacity){
           lock1.unlock();
           return false;
       }
       entries[tail % capacity ] = data ;
       tail++ ;
       lock1.unlock();       
    }

    int dequeue(){
        lock1.lock();
            if(tail == head ){
				lock1.unlock();
            	return -1;
            }
        int x = entries[head % capacity ];
		head ++ ;
        lock1.unlock();
		return x; 
    }
};
struct threadData{ 
    int id;
    Queue* dataQueue; 
};	

void* testCS(void* args){
	uniform_int_distribution<int> distribution(0, 1);
	struct threadData* tdata=(struct threadData*)(args);
    int id = tdata->id; // extract data from passed arguments
	Queue* qu = tdata->dataQueue;
	std::exponential_distribution<double>exponential2(1.0);
	for(int i=0;i<k;i++)
	{
		int choice = distribution(generator);
		struct timeval t_req,t_enter,t_exit,t_exitReq;
		int val = rand() % 100000000 ;
	
		time_t Request_time=time(NULL);
		gettimeofday(&t_req,NULL);// record request time       
		if(choice ==0 ){
			string FinalString1 = "Thr"+to_string(id+1) + " reqested enqueue of"+ to_string(val) + " for the "+ to_string(i) +  "th time on "+ to_string(t_req.tv_sec) + " "+ to_string(t_req.tv_usec) + "\n";
            fileLock.lock();
            output<<FinalString1;
            fileLock.unlock();
			bool successfull = qu->enqueue(val);
			gettimeofday(&t_exit,NULL);// record request time       
			string FinalString2 = "Thr"+to_string(id+1) + " successfully enqueued "+ to_string(val) + " for the "+ to_string(i) +  "th time on "+ to_string(t_exit.tv_sec) + " "+ to_string(t_exit.tv_usec) + "\n";

			if(successfull == 1){
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
			int val = qu->dequeue();
			gettimeofday(&t_exit,NULL);// record request time  
			string FinalString = "Thr"+to_string(id+1) + " successfully dequeued "+ to_string(val) + " for the "+ to_string(i) + "th time on "+ to_string(t_exit.tv_sec) + " "+ to_string(t_exit.tv_usec) + "\n";
			if(val != -1){
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
	Queue* qu = new Queue(10000000);
    for(int i=0;i<n;i++) // create n threads
    {
        struct threadData* tdata = new threadData{i,qu};            
      	pthread_create(&tid[i],&attr,testCS,(void*)(tdata));
		
    }

    for(int i=0;i<n;i++) // join threads
    {
        pthread_join(tid[i],NULL);
    }
    return 0;
}