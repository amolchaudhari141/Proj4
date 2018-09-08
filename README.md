# README #


CIS 620	Project 4: A distributed application using the Remote Procedure Call

### What is this repository for? ###

* This project is to write a distributed application using Remote Processure Call (RPC) protocal the pthread library and the CUDA toolkit


### Requirement Specifications ###
The project is divied into two parts:

**Client**

* provides interfaces to the users

* keeps a list of four available Linux workstations

* invokes a remote procedure getload() to collect the load average, and select the workstation with the lowest load average

* parse the command line, recognize the command:
	* -x, the program invokes findmax_gpu()
	* -u. the program invokes update_list()
	* both will return the result back

* use POSIX thread to mask the communication latency. (Question, how to pass the client handle to the thread?)
* set a 3-seconds timeout, use pthread_cancel() in the timeout handler routine
Haven't implement thread_cancel. Should work except if the server doesn't response immediately , will get segmentation fault. Expect pthread_cancel will solve this problem. But if you restart all the server, and get lucky, it should print out the correct result. 
**Server**

* provides three services:
	* system function call getloadavg(), to get the load average 
	* provides findmax_gpu() firstly initialize the large array using value N, M, and S passed from the client.
	* return the found largest element in the array and return the value
	* provides update_lst() getting a linked list of doubles from the client and then utilizes a higher-order to update the values in the list. 
	
**how to run**

* run the server program on workstations before starting the client
