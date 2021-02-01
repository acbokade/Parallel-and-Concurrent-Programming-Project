# Parallel-and-Concurrent-Programming-Project
Team:  
Ajinkya Bokade  
Atharva Sarage

Implemented Basket Queue (Ajinkya Bokade)
Basket Queue is a lock free linearizable highly concurrent linearizable FIFO queue. It maintains baskets
of mixed-order items instead of standard totally ordered list. It parallelizes enqueue operations among
different baskets by allowing different enqueue operations in different baskets to execute parallely. Nodes’
order in baskets isn’t specified while enqueuing.  
[Basket Queue Paper](https://people.csail.mit.edu/shanir/publications/Baskets%20Queue.pdf).  
  

Implemented Circular Queue (Atharva Sarage)
It is lock free linearizable concurrent FIFO queue based on the modified infinite array queue.
It manages cycles differently in dequeue, making it possible toleverage a simpler atomic OR operation instead of CAS.  
[Circular Queue Paper](https://drops.dagstuhl.de/opus/volltexte/2019/11335/pdf/LIPIcs-DISC-2019-28.pdf).
