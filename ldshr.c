#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include "ldshr.h"
#include <string.h>
#define NUM 4

// private global variables

static CLIENT *cl[NUM];  /* a client handl ptr list*/
// static char machinetab[NUM][10] = {"bach"};//, "chopin", "davinci", "spirit"};
// static double loadvag[NUM] = {100.0};//, 100,0, 100.0, 100.0};

static char machinetab[NUM][10] = {"bach", "chopin", "davinci", "degas"};
static double loadvag[NUM] = {100.0, 100.0, 100.0, 100.0};
pthread_mutex_t m;

// Sinal handler, after 3 seconds no response from server, this function is called
void handler(int sig)

{   
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	printf("thread call is called \n");
	pthread_cancel(pthread_self());
	return;
}

//a helper function call by pthread_create
void getload_helper(int j) {
	HEADNODE *doublePtr = malloc (sizeof(struct node));
	if(doublePtr==NULL){printf("error in mallocation\n"); exit(1);}
	HEADNODE *retDouble;
   
	// register signal handler inside the thread
	signal(SIGALRM, handler);
	alarm(3);
  
	retDouble = getload_1(doublePtr, cl[j]);
	loadvag[j] = retDouble->val;


}


main(argc, argv) 
	int argc;
	char *argv[];
{
	char *lookahead;
	node *next;
	if (argc < 2){
		fprintf(stderr, "Usage: %s -u or -x\n", argv[0]);
		exit(1);
	}


/****----------------------------Part one: get load -------------------------------------****/
	// create clnt handle for each workstation 
	int i;
	for (i = 0; i < NUM; i++){
         
		if(!(cl[i]= clnt_create(machinetab[i], LDSHRPROG, LDSHRVERS, "tcp"))) {
			clnt_pcreateerror(machinetab[i]);
			exit(1);
		}
	}
  

	int selected_idx; // hold the index for selected workstaion 
	pthread_t tid[NUM];


    int j;
   
 	for(j =0; j<NUM; j++){
 		int *anther = malloc(sizeof(int));
 		memcpy(anther, &j, sizeof(int));
		pthread_create(&tid[j], NULL, (void *) &getload_helper, *anther);

	}

    double MinLoad = 200.0;
    int k;
    for (k = 0; k < NUM; k++){
    	pthread_join(tid[k], NULL);
  
    }
    
    for (k = 0; k < NUM; k++){
    	printf("%s %f\n", machinetab[k], loadvag[k]);
    	if(MinLoad > loadvag[k]){   		
    		selected_idx = k;
    		MinLoad = loadvag[k];

    	}
    }
	printf("executed on %s\n", machinetab[selected_idx]);

   /****--------------------------Part two: find max of an exponential distibuted array---------****/
	lookahead = argv[1];
	if (strcmp(lookahead, "-x")==0) {
		if (!argc == 5){
			fprintf(stderr, "Usage: %s -x N M S\n",argv[0]);
			exit(1);
		}

		struct data_shape *shape = malloc(sizeof(struct data_shape));
		double *max_val;
		shape->N = atoi(argv[2]);
		shape->M = atoi(argv[3]);
		shape->S = atoi(argv[4]);

	
		max_val = findmax_gpu_1(shape, cl[selected_idx]);

		printf("%2f\n", *max_val);

	}


	// /****-----------------------Part three: map function on a linkedlist --------------------****/
	else if (strcmp(lookahead, "-u")==0){
		if(argc < 3){
			fprintf(stderr, "Usage: %s -u numberList\n", argv[0]);
			exit(1);
		}

		HEADNODE *headptr = malloc (sizeof(struct node));
		headptr->val = atoi(argv[2]);
		headptr->next = NULL;
        HEADNODE *current;
        current = headptr;

        //put the val into linkedlist
		int k;
		for (k = 3; k < argc ; k++){
			current->next = (struct node *) malloc (sizeof(struct node)); 
			current->next->val = atoi(argv[k]);
			current = current->next;
			current->next = NULL;
		}

        HEADNODE *retPtr;
        retPtr = update_lst_1(headptr, cl[0]);  
        retPtr = retPtr->next;
        // print the updated list
		while(retPtr!= NULL){
			printf("%1f ", retPtr->val);
			retPtr = retPtr->next;
		}
		printf("\n");

	}
	else {
		fprintf(stderr, "Wrong argument, Usage : %s -u or -x\n", lookahead);
		exit(1);
	}

}
	
