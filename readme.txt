1) Input to the program is a file named "inp-params.txt,".
   Input consists of the parameters n, k. n denotes the number of threads and k denotes the number of times each thread has to perform enqueue or dequeue operation.

2) Compile the Basket Queue code by executing following command:
   g++ -std=c++11 -pthread basket_queue.cpp -latomic -o basket_queue

3) Run the Basket Queue executable by :
   ./basket_queue < inp-params.txt

4) Compile the CircularQueue code by executing following command:
   g++ -std=c++11 -pthread circularQueue.cpp -latomic -o circularQueue

5) Run the CircularQueue executable by :
   ./circularQueue < inp-params.txt

6) Compile the Coarse Queue code by executing following command:
   g++ -std=c++11 -pthread coarse.cpp -latomic -o coarse

7) Run the Coarse Queue executable by :
   ./coarse < inp-params.txt

8) Output file 'output.txt' which contains the output logs of enqueue request, enqueue complete, dequeue request, dequeue complete.

9) For timing calculations printing time was not considered
