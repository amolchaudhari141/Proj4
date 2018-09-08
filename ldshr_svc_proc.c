#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include "ldshr.h"
#include <string.h>


extern void reduce_wrapper(int *n, int *mean, int *seed, double *max_val);
const int NELEM = 1; /* get load	 average over last 1 min */

void square_div(struct node *p){
	p->val = (p->val)*(p->val)/10.0;
}

/* A map function map each node's value to its squred value */
void map( void (*f) (struct node *p), HEADNODE *headptr) {
	printf("map function is called\n");
	while(headptr != NULL){
		(*f)(headptr);
		headptr = headptr->next;
	}
}
	
HEADNODE *
getload_1_svc(doublePtr, rqp)
HEADNODE *doublePtr;
struct svc_req *rqp;
{   printf("get called from client\n");
	double avg[3];	
	if(getloadavg(avg, NELEM) == -1){
			fprintf(stderr, "Fail to get load average\n");
			exit(1);   /* failed to return a load average*/
	}

	doublePtr->val = avg[0];
	// *loadavg = avg[0];
	printf("return value %f\n", doublePtr->val);

	return (doublePtr); /* succeeded to return a load average */
}

double *
findmax_gpu_1_svc(shape, rqp)
		struct data_shape *shape;
 		struct svc_req *rqp;
{
    //call findmax.cu
    printf("recieved call from client\n");
    static double *max_val;
    max_val = (double *) malloc(sizeof(double));
    reduce_wrapper(&shape->N, &shape->M, &shape->S, &max_val);
    return (&max_val);
}



HEADNODE *
update_lst_1_svc(headptr, rqp)
HEADNODE *headptr;
struct svc_req *rqp;
{ 
	HEADNODE *retPtr;
	retPtr = (HEADNODE *) malloc(sizeof(HEADNODE));
	retPtr = headptr;
	map(square_div, retPtr);
	// HEADNODE *q;
	// q = retPtr;
	// while (q!=NULL){
	// 	printf("check before return %f\n", q->val );
	// 	q = q->next;
	// }
	return (&retPtr);
}



