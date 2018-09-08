/*
*ldshr.x Open Network Computing Remote Process Call
*
*/
/*hard coding machine table*/


/* a linked list struct */
struct node {
	struct node *next;
	double val;
};

/* a struct to pass arguments to getload() function */



typedef struct node HEADNODE;

struct data_shape {
	int N;
	int M;
	int S;
};

/*program definition */
program LDSHRPROG{
	version LDSHRVERS{
		HEADNODE GETLOAD(HEADNODE) = 1; 

		double FINDMAX_GPU(struct data_shape) = 2;
		HEADNODE UPDATE_LST(HEADNODE) = 3;
	} = 1;
} = 0x2228012;
