#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<iostream>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
using namespace std;

struct PCB {
	int occupied; // either true or false
	pid_t pid; // process id of this child
	int startSeconds; // time when it was forked
	int startNano; // time when it was forked
	int endingTimeSeconds; // estimated time it should end
	int endingTimeNano; // estimated time it should end
};

const int BUFF_SZ = sizeof(int)*2;
int shm_key;
int shm_id;
struct PCB processTable[20];

int parse(int argc, char *argv[], int &proc, int &simul, float &time_limit, float &interval);

void incrementClock(int *sec, int *nano);

int *initializeMemory();

int main(int argc, char* argv[]) {
	int *clock = initializeMemory();
	int *sec = &(clock[0]);
	int *nano = &(clock[1]);
	*sec = *nano = 0;

	int proc_running = 0, total = 0;
	int status;
	int proc;
	int simul;
	float time_limit;
	float interval;

	parse(argc, argv, proc, simul, time_limit, interval);

	cout << "OSS starting PID: " << getpid() << " PPID: " << getppid() << endl
	<< "Called with:" << endl
	<< "-n " << proc << endl
	<< "-s " << simul << endl
	<< "-t " << time_limit << endl
	<< "-i " << interval << endl;

	while(total < proc) { //When the amount of processes ran reaches the amount of processes given it will stop running more processes
		incrementClock(sec, nano); 
        	if(proc_running < simul) { //This controls how many processes are running at a single time
                	int pid = fork(); //creates copy of process that runs at the same time as eachother
                	if (pid == -1) {
                        	cout <<"Failed to launch child" << endl;
                        	exit(1);
                	}
                	else if (pid == 0) { //seperates child copy from parent copy
				int seconds = time_limit;
                                int nano_seconds = (time_limit - seconds) * 1000000000;
                        	cout << "Child has been launched, its pid is: " << getpid() << endl; //announcing			 		
                        	execlp("./worker","./worker",to_string(seconds).c_str(), to_string(nano_seconds).c_str(), (char*) NULL); //calls child and ends
                	}
                	proc_running++; //parent keeps count of processes running and total
                	total++;
        	}
        	else { //If the processes running currently goes over the simul limit given the parent will wait until it is done
                        cout << "Hello I am the parent, my pid is: " << getpid() << endl;
                	//put wait call here? 
                        proc_running = proc_running - 1;
        	}
	}

	//output summary report of how many total processes finished
	while(wait(&status) != -1){}
		cout << total << " total processes finished." << endl;
	
	shmdt(clock); // Detach from the shared memory segment
	shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id
	return 0;
}

void incrementClock(int *sec, int *nano) {
    *nano += 10000000; // amount of nano seconds per loop
    if (*nano >= 1000000000) {
        (*sec)++;
        *nano -= 1000000000;
    }
}

int *initializeMemory() {
	int shm_key = ftok("oss.c",0);
	if (shm_key <= 0) {
		fprintf(stderr,"Parent:... Error in ftok\n");
		exit(1);
	}
	shm_id = shmget(shm_key,BUFF_SZ,0700|IPC_CREAT);
	if (shm_id <= 0 ) {                                                                                                                                                 fprintf(stderr,"Parent:... Error in shmget\n");                                                                                                             exit(1);                                                                                                                                            }                                                                                                                                                           int *clock = (int *)shmat(shm_id,0,0);                                                                                                                      if (clock <= 0) {                                                                                                                                                   fprintf(stderr,"Parent:... Error in shmat\n");                                                                                                              exit(1);
        }
	return clock;
}

int parse(int argc, char *argv[], int &proc, int &simul, float &time_limit, float &interval) { //uses arguments as reference to have default arguments
        int opt;
        proc = 1; //processes
        simul = 1; //simulations
        time_limit = 1; //time limit to launch children
	interval = 1;	//interval 1 in seconds to launch children
        while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) { //goes through each paramater h,n,s,t,i and gets the value which is argv holds the parameter, argv holds how many
        	switch (opt) { //each parameter goes in and gets chosen by its equivalent case
                	case 'h':
                        	cout << "oss [-h] [-n proc] [-s simul] [-t time_limit] [-i interval]" << endl
                        	<< "The proc parameter stands for number of total children to launch," << endl  
				<< "the simul parameter indicates how many children to allow to run simultaneously." << endl
				<< "time limit is the simulated time (seconds) each child runs before terminating; can be a float" << endl
				<< "interval is the minimum time (seconds) between launching children; can be a float (e.g. 0.1 = 100 ms)" << endl
                        	<< "./oss -n 5 -s 3 -t 4 -i 0.2" << endl;
				exit(0);
                	case 'n':
                        	proc = atoi(optarg);
                        	break;
                	case 's':
          	             	simul = atoi(optarg);
                        	break;
                	case 't':
                       		time_limit = atof(optarg);
                        	break;
			case 'i':
                        	interval = atof(optarg);
                        	break;
                	default: //if there is an unexpected value
                        	cout << "Unexpected value incountered " << atoi(optarg) << endl;
                        	break;
        	}
	}	
return 0;
}
