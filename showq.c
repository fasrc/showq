/* Libraries needed by this code */
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <lsf/lsbatch.h>
#include <lsf/lsf.h>
#include <getopt.h>

/* Struct designed to help pass data in and out of subroutines.  This struct contains information about the number of cores and nodes used by a queue being polled by the user */
struct numq {
	int cores;
	int nodes;
};

/* Prints out information about the jobs running that fit the defined parameters */
struct numq print_section( int options, char *inp_queue, char *user, char *inp_hosts, int numcoresq, int numnodesq) {

	/* variables for simulating bjobs command */
  	struct jobInfoEnt *job;     /* detailed job info */
  	struct queueInfoEnt *que;   /* detailed queue info */
  	struct numq nq;	      /* information about the queue being polled by the user that we wish to pass on to other routines */
  	int tot_jobs;               /* total jobs */
  	int more;                   /* number of remaining jobs unread */
  	int nodenum;                /* Node number counter */
  	int i,j;		      /* counter*/
  	int alreadycounted;	      /* Logical flag used to figure out if the quantity in question has already been counted*/
  	int numQueues = 1;	      /* number of Queues to poll queue info about */
  	int t;		      /* Time remaining */
  	int tdays;		      /* Time remaining in days */
  	int thrs;		      /* Time remaining in hours */
  	int tmin;		      /* Time remaining in minutes */
  	int tsec;		      /* Time remaining in seconds */	
  	char *targetqueue;       /* array for the queue names to poll queue info about */
  	char startstr[14];          /* string of date when job started */
  	char submitstr[14];         /* string of data when job was submitted */
  	char statstr[5];            /* string of status */  
  	char exclus;		      /* exclusive flag */
  	char nodelistcheck[10000][20]; /* a list of node names that we will compare against to test uniqueness*/
  
  	/* Initialize struct values */
  	nq.cores=numcoresq;
  	nq.nodes=numnodesq;
  
  	/* gets the total number of jobs. Exits if failure */
  	tot_jobs = lsb_openjobinfo(0, NULL, user, inp_queue, inp_hosts, options);
  
  	/* Sanity Checks */
  	if (tot_jobs < 0) { 
    		printf("No matching jobs found\n");
    		return nq;
	}

  	if (tot_jobs<0){ 
    		lsb_perror("lsb_openjobinfo");
    		exit(-1);
	}

  	/* Print header for section */
  	printf("%-12s %-8.8s %-6.6s %-7.7s %-4s  %-14s   %-14s %-14s\n",	\
		 "JOBID", "USER", "STAT", "QUEUE", "CORES/NODES", "TIME REMAINING",		\
		 "SUBMIT TIME", "START TIME");

  	/* Loop over jobs until complete */
  	for (;;) {
    		job = lsb_readjobinfo(&more);   /* get the job details */

    		/* Sanity Check */
    		if (job == NULL) {
      			lsb_perror("lsb_readjobinfo");
      			exit(-1);
    		}

    		/* Store our current target queue */
    		targetqueue=job->submit.queue;

    		/* Grab information about that queue */
    		que = lsb_queueinfo(&targetqueue,&numQueues,NULL,NULL,0);


    		/* Sanity check */
    		if (que == NULL){
			lsb_perror("lsb_queueinfo");
			exit(-1);
    		}


    		/* Detects if the job is running in exclusive mode */
    		exclus=' ';
    		if (job->submit.options & SUB_EXCLUSIVE) {
			exclus='X';
		}

    		/* Counts the Number of nodes */
    		nodenum=1;
    		if (job->numExHosts > 0) {
			for(i=0;i < job->numExHosts-1; i++) {
				if(strcmp(job->exHosts[i],job->exHosts[i+1]) != 0){
					nodenum++; 
				}
			}
		}

    		/* Finds the jobs status */
    		if (job->status == JOB_STAT_RUN){
			strcpy(statstr,"RUN");
			}
    		else if(job->status == JOB_STAT_PEND){
      			strcpy(statstr,"PEND");
			} 
    		else if(job->status == JOB_STAT_PSUSP){
      			strcpy(statstr,"PSUSP");
			} 
    		else if(job->status == JOB_STAT_SSUSP){
      			strcpy(statstr,"SSUSP");
		}

    		/* Test if there is a queue that the user is interested in */
    		if (inp_queue != NULL) {

			/* Stores the number of processors and nodes used by this queue for later use */
			if (job->status == JOB_STAT_RUN && strcmp(inp_queue,targetqueue) == 0) {
				numcoresq=job->submit.numProcessors+numcoresq;

				/* Count the number of unique nodes are used by this queue */
				for (i=0;i < job->numExHosts;i++){
					/* We need to check that we aren't double counting nodes */
					alreadycounted=0;
	
					for(j=0;j<numnodesq;j++){
						if (strcmp(job->exHosts[i],nodelistcheck[j]) == 0) {
							alreadycounted=1;
							break;
						}
					}

					/* If we've already been counted break out of this loop if not counted then store that data */
					if (alreadycounted == 0) {
						strcpy(nodelistcheck[numnodesq],job->exHosts[i]);
						numnodesq++;
					}
				}
			}
		}

    		/* Figures out what time the job was submitted at and when it started running*/    
    		strftime(submitstr,14,"%b %d %R",&(*localtime(&job->submitTime)));
    		strftime(startstr,14,"%b %d %R",&(*localtime(&job->startTime)));

    		/* Calculates Time Remaining */
    		t=que->rLimits[LSF_RLIMIT_RUN]-job->runTime;

    		/* Checks to see if ther termination time is other than the queue limit */
    		if (job->submit.rLimits[LSF_RLIMIT_RUN] > 0) {
			t=job->submit.rLimits[LSF_RLIMIT_RUN]-job->runTime;
		}

    		/* Calculates the Time remaining in Days, Hours, Minutes and Seconds */
    		tdays=t/24/60/60;

    		thrs=t/60/60-24*tdays;

    		tmin=t/60-24*60*tdays-60*thrs;

    		tsec=t-24*60*60*tdays-60*60*thrs-60*tmin;

    		/* Prints out job data */	
    		printf("%-12s %-8.8s %-6.6s %-8.8s %4i%1s%2i%1c       %2i%1s%-2.2i%1s%2.2i%1s%2.2i  %-14s",	\
    			lsb_jobid2str(job->jobId), job->user, statstr, job->submit.queue, \
    			job->submit.numProcessors, "/",nodenum,exclus, tdays,":", thrs,":",tmin,":",tsec ,submitstr);

    		/* Prints out start time if the job is not pending */
    		if (options != PEND_JOB) {
			printf("%-14s",startstr);
		}
	
    		printf ("\n");

    		/* Checks to see if there are more jobs to do. */	
    		if (!more) break;
    	}

	/* Prints the total number of jobs in the section */
  	printf("%-i total jobs\n",tot_jobs);

	/* Store number of cores and nodes used by the queue.  These will have nonzero values if the queue is the one being polled by the user. */
  	nq.cores=numcoresq;
  	nq.nodes=numnodesq;

	/* Transmit that data out of the function */
  	return nq;

}

/* Summarizes aspect of cluster currently being polled for jobs */
void print_summary(char *inp_queue, char *user, char *inp_hosts, int numcoresq, int numnodesq) {
	struct jobInfoEnt *job;     /* detailed job info */
	struct queueInfoEnt *que;   /* detailed queue info */
	struct queueInfoEnt *que2;   /* detailed queue info */
	struct hostInfoEnt *hos;    /* detailed host info */
  	int numQueues, numQueues2;  /* number of Queues to query about */
	int numHosts; /* number of Hosts */
	int i,j,k,ii; /* counters */
	int totHosts=0, totClosedAdm=0, totUnavail=0, totUsed=0; /* counters for total number of nodes, total closed by the Admin, total Unavailable and total used*/
	int totCores=0, totCoresUsed=0; /* counters for total number of cores and number of cores used. */
	float perutil; /* work variable */
	char *hostlist;         /* Array of Hosts */
	char quehostlist[1000]; /* list of hosts for a specific queue */
	char *token; /* Used for parsing */
	char *delimiters = " "; /* Used for parsing */
	char *slash = "/"; /* Used for parsing */
	char *saveptr1; /* Used for parsing */
	char queuelistcheck[100][20]; /* Stores what queues we have already checked */
	char hostlistcheck[10000][20]; /* Stores what hosts we have already checked */
	char hostscheck[100000][20];  /* Stores what hostlists we have alredy checked */
	int numhlc,numhc,numoqc, alreadycounted; /* Number of entires for the above arrays plus a logical for whether or not the variable has already be checked. */


	/* If there is a queue we are pooling use this summary */
	if (inp_queue != NULL) {
		printf("Queue Summary\n");

		/* First let's get information on the queue in question */
		numQueues=1;
		que = lsb_queueinfo(&inp_queue,&numQueues,NULL,NULL,0);

		/* Copy the hostlist to the a temporary file */
		strcpy(quehostlist,que->hostList);

		/* Iterate through the host list which is seperated by spaces */

		numoqc=0;

		i=0;

		for(;;) {
			/* Parses the hostlist based on the defined delimiters */
			if (i == 0){
				token = strtok(quehostlist,delimiters);
				}
			else{
				token = strtok(NULL,delimiters);
			}

			if (token == NULL) break;

			/* We need to trim off slashes */
			saveptr1=strpbrk(token,slash);

			if (saveptr1 != NULL) {
				token[strlen(token)-1]='\0';
			}
				

			/* We will first poll the hosts available to this queue and see what is going on */

			hostlist=token;

			numHosts=1;

			hos = lsb_hostinfo(&hostlist, &numHosts);

			totHosts=totHosts+numHosts;

			/* Now we are going to check each host for what state it is in */
			/* We will also collect statistics regarding the usage of the host at the same time */
			for (j=0;j<numHosts;j++){
				if (hos[j].hStatus & HOST_STAT_UNLICENSED) {
      					}/* unlicensed */
				else if (hos[j].hStatus & HOST_STAT_UNAVAIL) {
					totUnavail++; /* LIM and sbatchd are unavailable */
					}
    				else if (hos[j].hStatus & HOST_STAT_UNREACH) {
      					totUnavail++; /* sbatchd is unreachable */
					}
    				else if (hos[j].hStatus & HOST_CLOSED_BY_ADMIN) {
					totClosedAdm++; /* host closed by administrator */
					}
				else if (hos[j].hStatus & ( HOST_STAT_WIND | HOST_STAT_DISABLED | HOST_STAT_NO_LIM)) {
      					}
    				else if (hos[j].hStatus & ( HOST_STAT_BUSY | HOST_STAT_FULL | HOST_STAT_LOCKED )) {
					/* Host is full */
					totUsed++;
					totCoresUsed=hos[j].numRUN+totCoresUsed;
					totCores=hos[j].maxJobs+totCores;
					}
				else {
					/* Host is partially used */
					if (hos[j].numRUN > 0) {
						totUsed++;
					}

					totCoresUsed=hos[j].numRUN+totCoresUsed;
					totCores=hos[j].maxJobs+totCores;
				}
			}


			/* Check what other queues are pointing to this host */
			numQueues2=0;

			que2 = lsb_queueinfo(NULL,&numQueues2,hos->host,NULL,0);

			for (k=0;k<numQueues2;k++){
				/* Check and see that the queue is not the one we are currently interested in */
				if (strcmp(inp_queue,que2[k].queue) != 0) {

					/* We need to check that we aren't double counting queues */
					alreadycounted=0;

					for(ii=0;ii<numoqc;ii++){
						if (strcmp(que2[k].queue,queuelistcheck[ii]) == 0) {
							alreadycounted=1;
							break;
						}
					}

					/* If we've already been counted break out of this loop if not counted then store that data */
					if (alreadycounted == 0) {
						strcpy(queuelistcheck[numoqc],que2[k].queue);
						numoqc++;
					}
				}

			}

			i=i+1;
		}


		/* Now to print what we found */
		/* First Data on this Specific Queue */
		/* Calculate the percentage of cores used */
		perutil=(float)numcoresq/(float)totCores*100;

		printf("%4i of %4i Cores Used (%4.2f%%)\n", numcoresq, totCores, perutil);

		/* Calculate the percentage of nodes used */
		perutil=(float)numnodesq/(float)totHosts*100;

		printf("%4i of %4i Nodes Used (%4.2f%%)\n", numnodesq, totHosts, perutil);

		/* Now for all the nodes accessible by the queue */
		printf("\n");

		printf("Overall Statistics for Nodes Available to this Queue\n");

		/* Calculate the percentage of cores used */
		perutil=(float)totCoresUsed/(float)totCores*100;

		printf("%4i of %4i Cores Used (%4.2f%%)\n", totCoresUsed, totCores, perutil);

		/* Calculate the percentage of nodess used */
		perutil=(float)totUsed/(float)totHosts*100;

		printf("%4i of %4i Nodes Used (%4.2f%%), %2i Nodes Closed by Admin, %2i Nodes Unavailable\n", totUsed, totHosts, perutil, totClosedAdm, totUnavail);

		printf("\n");

		/* Print out which other queues can submit to these hosts */

		printf("Other Queues Which Submit To The Hosts For This Queue: ");

		j=0;

		for (k=0;k<numoqc;k++) {
			if (k == numoqc-1) {
				printf("%4s", queuelistcheck[k]);
				}
			else {
				printf("%4s, ", queuelistcheck[k]);
			}

			j++;
			/* Word wraps if line is too long */
			if (j == 4) {
				printf("\n");
				j=0;
			}
		}

		printf("\n");

		}
	else if (inp_hosts != NULL) {
		printf("Host Summary\n");
		/* First let's get information on the host in question */

		hostlist=inp_hosts;

		numHosts=1;

		hos = lsb_hostinfo(&hostlist, &numHosts);

		totHosts=totHosts+numHosts;

		/* Now we are going to check each host for what state it is in */
		/* We will also collect statistics regarding the usage of the host at the same time */
		for (j=0;j<numHosts;j++){
			if (hos[j].hStatus & HOST_STAT_UNLICENSED) {
      				}/* unlicensed */
			else if (hos[j].hStatus & HOST_STAT_UNAVAIL) {
				totUnavail++; /* LIM and sbatchd are unavailable */
				}
    			else if (hos[j].hStatus & HOST_STAT_UNREACH) {
      				totUnavail++; /* sbatchd is unreachable */
				}
    			else if (hos[j].hStatus & HOST_CLOSED_BY_ADMIN) {
				totClosedAdm++; /* host closed by administrator */
				}
			else if (hos[j].hStatus & ( HOST_STAT_WIND | HOST_STAT_DISABLED | HOST_STAT_NO_LIM)) {
      				}
    			else if (hos[j].hStatus & ( HOST_STAT_BUSY | HOST_STAT_FULL | HOST_STAT_LOCKED )) {
				/* Host is full */
				totUsed++;
				totCoresUsed=hos[j].numRUN+totCoresUsed;
				totCores=hos[j].maxJobs+totCores;

				}
			else {
				/* Host is partially used */
				if (hos[j].numRUN > 0) {
					totUsed++;
				}

				totCoresUsed=hos[j].numRUN+totCoresUsed;
				totCores=hos[j].maxJobs+totCores;
			}

		}

		/* Check what queues are pointing to this host */
		numoqc=0;
		numQueues2=0;
		que2 = lsb_queueinfo(NULL,&numQueues2,hos->host,NULL,0);

		for (k=0;k<numQueues2;k++){
			/* We need to check that we aren't double counting queues */
			alreadycounted=0;

			for(ii=0;ii<numoqc;ii++){
				if (strcmp(que2[k].queue,queuelistcheck[ii]) == 0) {
					alreadycounted=1;
					break;
				}
			}

			/* If we've already been counted break out of this loop if not counted then store that data */
			if (alreadycounted == 0) {
				strcpy(queuelistcheck[numoqc],que2[k].queue);
				numoqc++;
			}
		}
		

		/* Now to print what we found */
		/* Calculate the percentage of cores used */
		perutil=(float)totCoresUsed/(float)totCores*100;

		printf("%4i of %4i Cores Used (%4.2f%%)\n", totCoresUsed, totCores, perutil);

		/* Calculate the percentage of nodes used */
		perutil=(float)totUsed/(float)totHosts*100;

		printf("%4i of %4i Nodes Used (%4.2f%%), %2i Nodes Closed by Admin, %2i Nodes Unavailable\n", totUsed, totHosts, perutil, totClosedAdm, totUnavail);

		printf("\n");

		/* List the queues that access these hosts */

		printf("Queues That Run On These Hosts: ");

		j=0;

		for (k=0;k<numoqc;k++) {
			if (k == numoqc-1) {
				printf("%4s", queuelistcheck[k]);
				}
			else {
				printf("%4s, ", queuelistcheck[k]);
			}

			/* Word wrap if list is too long */
			j++;
			if (j == 8) {
				printf("\n");
				j=0;
			}
		}

		printf("\n");
		}
	else if (inp_queue == NULL && inp_hosts == NULL) {
		printf("User Summary\n");

		/* First lets get information on the Queues the User has access to */

		numQueues=0;
		que = lsb_queueinfo(NULL,&numQueues,NULL,user,0);

		numhc=0;
		numhlc=0;

		/* Iterate through the queue list */
		for(k=0;k<numQueues;k++) {

			/* Iterate through the host list which is seperated by spaces */

			i=0;

			for(;;) {

				/* Parse the host list based on predefined delimiters */
				if (i == 0){
					token = strtok(que[k].hostList,delimiters);
					}
				else{
					token = strtok(NULL,delimiters);
				}

				if (token == NULL) break;

				/* We need to trim off slashes */
				saveptr1=strpbrk(token,slash);

				if (saveptr1 != NULL) {
					token[strlen(token)-1]='\0';
				}
				

				/* We will first poll the hosts available to this queue and see what is going on */

				hostlist=token;

				/* We need to check that we aren't double counting host groups */
				alreadycounted=0;

				for(j=0;j<numhlc;j++){
					if (strcmp(hostlist,hostlistcheck[j]) == 0) {
						alreadycounted=1;
						break;
					}
				}

				/* If we've already been counted break out of this loop if not counted then store that data */
				if (alreadycounted == 0) {
					strcpy(hostlistcheck[numhlc],hostlist);
					numhlc++;
					}
				else {
					break;
				}

				/* Query for hostlist information */
				numHosts=1;

				hos = lsb_hostinfo(&hostlist, &numHosts);

				/* Now we are going to check each host for what state it is in */
				/* We will also collect statistics regarding the usage of the host at the same time */
				for (j=0;j<numHosts;j++){
					/* Some hosts lists contain redundant hosts so to make sure we aren't double counting we will check again */

					alreadycounted=0;

					for(ii=0;ii<numhc;ii++){
						if (strcmp(hos[j].host,hostscheck[ii]) == 0) {
							alreadycounted=1;
							break;
						}
					}

					/* If we've already been counted break out of this loop if not counted then store that data */
					if (alreadycounted == 0) {
						strcpy(hostscheck[numhc],hos[j].host);
						numhc++;
						}
					else {
						break;
					}

					totHosts++;

					if (hos[j].hStatus & HOST_STAT_UNLICENSED) {
      						} /* unlicensed */
					else if (hos[j].hStatus & HOST_STAT_UNAVAIL) {
						totUnavail++; /* LIM and sbatchd are unavailable */
						}
    					else if (hos[j].hStatus & HOST_STAT_UNREACH) {
      						totUnavail++; /* sbatchd is unreachable */
						}
    					else if (hos[j].hStatus & HOST_CLOSED_BY_ADMIN) {
						totClosedAdm++; /* host closed by administrator */
						}
					else if (hos[j].hStatus & ( HOST_STAT_WIND | HOST_STAT_DISABLED | HOST_STAT_NO_LIM)) {
      						}
    					else if (hos[j].hStatus & ( HOST_STAT_BUSY | HOST_STAT_FULL | HOST_STAT_LOCKED )) {
						/* Host is full */
						totUsed++;
						totCoresUsed=hos[j].numRUN+totCoresUsed;
						totCores=hos[j].maxJobs+totCores;

						}
					else {
						/* Host is partially used */
						if (hos[j].numRUN > 0) {
							totUsed++;
						}
	
						totCoresUsed=hos[j].numRUN+totCoresUsed;
						totCores=hos[j].maxJobs+totCores;
					}
				}
	
				i=i+1;
			}
		}

		printf("Overall Statistics for Available Queues\n");

		/* Now to print what we found */
		/* Calculate the percentage of cores used */
		perutil=(float)totCoresUsed/(float)totCores*100;

		printf("%4i of %4i Cores Used (%4.2f%%)\n", totCoresUsed, totCores, perutil);

		/* Calculate the percentage of nodes used */
		perutil=(float)totUsed/(float)totHosts*100;

		printf("%4i of %4i Nodes Used (%4.2f%%), %2i Nodes Closed by Admin, %2i Nodes Unavailable\n", totUsed, totHosts, perutil, totClosedAdm, totUnavail);

		printf("\n");

		/* List what queues are available to the user */
		printf("Available Queues: ");

		j=0;

		for (k=0;k<numQueues;k++) {
			if (k == numQueues-1) {
				printf("%4s", que[k].queue);
				}
			else {
				printf("%4s, ", que[k].queue);
				}
			j++;
			/* Word wrap if list is too long */
			if (j == 8) {
				printf("\n");
				j=0;
			}
		}

		printf("\n");
	}
}


/* Main routine. */
int main(int argc, char **argv) {
 	struct numq nq; /* Struct for passing information between routines */
  	int c; /* Used for holding information passed in from the command line */
  	char *user = "all", *queue=NULL, *hosts=NULL; /* Pointers to the information we are querying */
  	int i, def; /* counters */


  	/* Initialize Default Flag and structs*/
  	def=1;
  	nq.cores=0;
  	nq.nodes=0;

	/* Read in and interpret command line options */
  	while (1) {
      		static struct option long_options[] =
		{
	  		/* These options set a flag. */
	  		{"help", no_argument,0, 'h'},
	  		{"q",  required_argument, 0, 'q'},
	  		{"u",  required_argument, 0, 'u'},
	  		{"n",  required_argument, 0, 'n'},
	  		{0, 0, 0, 0}
		};
      		/* Get information from command line */
      		int option_index = 0;
     
      		c = getopt_long (argc, argv, "u:q:n:help",long_options, &option_index);
     
      		/* Detect the end of the options. */
      		if (c == -1) break;

		/* Check what options we are using */
      
      		/* Default Flag set to false, we are actually passing options so the default is not used */
      		def=0;

      		switch (c) {
		case 0:
	  		/* If this option set a flag, do nothing else now. */
	  		if (long_options[option_index].flag != 0) break;
	  		printf ("option %s", long_options[option_index].name);
	  		if (optarg) printf (" with arg %s", optarg);
	  		printf ("\n");
	  		break;

		case 'q':
	  		queue = optarg;
	  		break;
	  
		case 'u':
	  		user = optarg;
	  		break;

		case 'n':
	  		hosts = optarg;
	  		break;

		case '?':
	  		/* This section handles when getopt_long is confused about what you are passing it */
	  		/* getopt_long already printed an error message. */
	  		exit(-1);

		case 'h':
	  		/* Displays this help page then exits */
	  		printf("Welcome to the Help Section.\n");
	  		printf("Showq provides an overview of the cluster and LSF queues in a similar way to showq in PBS.Torque.\n");
	  		printf("By default showq will show you your running, pending and suspended jobs plus an overview of the cluster.\n");
	  		printf("Depending on cluster activity this command could take a few minutes to complete.\n");
	  		printf("Each job has several fields which are listed below.\n");
	  		printf("\n");
	  		printf("JOBID           The LSF job id.\n");
	  		printf("\n");
	  		printf("USER            The name of the user.\n");
	  		printf("\n");
	  		printf("STAT            The current job state.  RUN if it is running, PEND if it is pending,\n");
	  		printf("                PSUSP if suspended by the owner or admin,\n");
	  		printf("                SSUSP if suspended due to host being overloaded or queue run window closure.\n");
	  		printf("\n");
	  		printf("QUEUE           The LSF queue the job was submitted from.\n");
	  		printf("\n");
	  		printf("CORES/NODES     This lists the number of cores used by the process followed by the number of nodes after the /.\n");
	  		printf("                Additionally if the job is running in Exclusive mode a X appears next to the node count.\n");
	  		printf("\n");
	  		printf("TIME REMAINING  The amount of time remaining on this run.  This is based off of the queue run time limit\n");
	  		printf("                or if the user defines a run time using BSUB -W it will use that.  Jobs that overrun\n");
	  		printf("                the a time limit show up with negative time equal to the amount they have overrun.\n");
	  		printf("                Jobs from queues with no time limit and no user defined limit show up as negative as well.\n");
	  		printf("                The value in that case is just the negative value of the time the job has run for.\n");
	  		printf("\n");
	  		printf("SUBMIT TIME     The time at which the job was submitted to the queue.\n");
	  		printf("\n");
	  		printf("START TIME      The time at which the job started running.\n"); 
	  		printf("\n");
	  		printf("Additional Usage options are as below.\n");
	  		printf("\n");
	  		printf("-help           Gets you to this convenient help page.\n");
	  		printf("\n");
	  		printf("-q queue_name   Shows you what is going on in that queue currently.\n");
	  		printf("\n");
	  		printf("-u user_name    Shows the jobs current associated with that user.\n");
	  		printf("                By default this is set to all if -n or -q is used other wise it is set to the current user.\n");
	  		printf("                If -u is used in tandem with -q it will show only the jobs running on that queue for that user.\n");
	  		printf("\n");
	  		printf("-n host_name    Shows what is going on for this host or group of hosts.  Does not show pending jobs for these hosts.\n");
	  		exit(-1);
	  
		default:
	  		abort ();
		}
	}

  	/* Sets what will happen when the default flag is set */
  	/* In this case it will simply return what jobs belong to the user and the node summary */
  	if (def) user = NULL;
  
  	/* initialize LSBLIB  and  get  the  configuration environment */
	if (lsb_init(argv[0]) < 0) {
    		lsb_perror("simbjobs: lsb_init() failed");
    		exit(-1);
  	}

	/* Query about the following information */
  	printf("ACTIVE JOBS-------------\n");
  	nq = print_section(RUN_JOB, queue, user, hosts, nq.cores, nq.nodes);

  	printf("\nSUSPENDED JOBS-------------\n");
  	nq = print_section(SUSP_JOB, queue, user, hosts, nq.cores, nq.nodes);

  	printf("\n");
  	print_summary(queue, user, hosts, nq.cores, nq.nodes);

  	printf("\n");
  	printf("PENDING JOBS-------------\n");
  	nq = print_section(PEND_JOB, queue, user, hosts, nq.cores, nq.nodes);

  	/* when finished to display the job info, close the connection to the mbatchd */
	lsb_closejobinfo();
  
	exit(0);
}
