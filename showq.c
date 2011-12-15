/* Libraries needed by this code */
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <lsf/lsbatch.h>
#include <lsf/lsf.h>
#include <getopt.h>

/* Calculates statistics about the cluster */
void print_nodes(int *who_flag, char *queue, char *hosts) {

  struct hostInfoEnt *hInfo;
  int i, t, all_hosts=0, num_cores=0, num_hosts=0, run_cores=0;
  int iliad_all_hosts=0, iliad_num_cores=0, iliad_num_hosts=0;
  int iliad_part_cores=0, iliad_empty_cores=0, iliad_tot_empty_cores=0;
  int iliad_part_hosts=0, iliad_run_cores=0, iliad_empty_hosts=0, iliad_full_hosts=0;
  int hero_all_hosts=0, hero_num_cores=0, hero_num_hosts=0;
  int hero_part_cores=0, hero_empty_cores=0, hero_tot_empty_cores=0;
  int hero_part_hosts=0, hero_run_cores=0, hero_empty_hosts=0, hero_full_hosts=0;
  int pp_all_hosts=0, pp_num_cores=0, pp_num_hosts=0;
  int pp_part_cores=0, pp_empty_cores=0, pp_tot_empty_cores=0;
  int pp_part_hosts=0, pp_run_cores=0, pp_empty_hosts=0, pp_full_hosts=0;
  float iliad_frac_used=0.0, hero_frac_used=0.0;
  char buffer[64]="\0", saveme[64];
  char *token, *token1, *user;
  struct hostInfoEnt *hostinfo2;
  struct queueInfoEnt *qInfo;
  int numqueues, numhosts=0, numusers=0, tot_hosts=0, tot_group_users=0, tot_users=0, num_preempt_queues=0;
  char *queueuser = NULL, *queuehost = NULL, *userlist=NULL;
  char *delimiters = " ", *delimiters1 = " []", preempt_queues[8192];
  char **hosts1=NULL;
  struct groupInfoEnt *groupinfo;
  const int NUMHOSTS_MAX=8192;
  const int NUMUSERS_MAX=8192;
  const char inp_user="USER";
  char *saveptr1, *saveptr2;
  
  if ( queue == NULL ) {
    numhosts = 0;
    hInfo = lsb_hostinfo(hosts1, &numhosts);
    printf("\n\nCluster Summary:");
  }    
  else {
    token = (char *)malloc(sizeof(char)*128);
 
    if ((hosts1 = (char **)malloc(sizeof(char *)*NUMHOSTS_MAX)) == NULL)
      {
	fprintf(stderr, "Unable to allocate memory for usernames\n");
	return -1;
      }
 
    if ((hosts1[0] = (char *)malloc(sizeof(char)*NUMHOSTS_MAX*MAXHOSTNAMELEN)) == NULL)
    {
      fprintf(stderr, "Unable to allocate memory for usernames[0]\n");
      return -1;
    }
 
    for (i=1;i<NUMHOSTS_MAX;i++) {
      hosts1[i] = hosts1[i-1]+MAXHOSTNAMELEN;
    }

    /*    preempt_queues = (char *) malloc(sizeof(char)*8192);*/

    numqueues = 1;
    qInfo = lsb_queueinfo(&queue, &numqueues, queuehost, queueuser, 0);

    
    /* if command line flag -w is set, collect users that can submit to this queue */
    if (who_flag == 1) {

      userlist = (char *) malloc(sizeof(char)*8192);

      for (;;) {
	
	if (tot_users == 0) { 
	  /* split hostList */
	  token = strtok_r(qInfo->userList, delimiters, &saveptr1);
	}
	else {
	  token = strtok_r(NULL, delimiters, &saveptr1);
	}
	
	if(token == NULL)
	  break;
	
	if (strchr(token, '/')) { 
	  /* if user is a usergroup, send it through lsb_usergrpinfo */

	  numusers = 1;
	  token[strlen(token)-1]='\0';
	  groupinfo = lsb_usergrpinfo(&token, &numusers, 0);
	  tot_group_users = 0;
	  for (;;) {
	    
	    if (tot_group_users == 0) { 
	      /* split hosts in queue */
	      token1 = strtok_r(groupinfo->memberList, delimiters, &saveptr2 );
	    }
	    else {
	      token1 = strtok_r(NULL, delimiters, &saveptr2);
	    }

	    if(token1 == NULL)
	      break;
	    
	    if (tot_group_users == 0) { 
	      strcpy(userlist, token1);
	    }
	    else
	      {strcat(userlist, token1 );}

	    strcat(userlist, ", ");
	    
	    tot_users++;
	    tot_group_users++;
	  }
	}
	else
	  {
	    strcat(userlist, token);
	    strcat(userlist, ", ");
	    tot_users++;
	  }
      }
      userlist[strlen(userlist)-2]='.';
    }
    
    /* collect queues that can be preempted by this one */
    token = strtok(qInfo->preemption, delimiters1); /* throw away first match */

    for (;;) {
      
      token = strtok(NULL, delimiters1);
      if(token == NULL)
	break;
      
      if(num_preempt_queues == 0) 
	{strcpy(preempt_queues, token);}
      else
	{strcat(preempt_queues, token);}

      strcat(preempt_queues, ", ");
      
      num_preempt_queues++;
    }

    preempt_queues[strlen(preempt_queues)-2] = '.';
    
    /* collect hosts in this queue */
    for (;;) {
      
      if (tot_hosts == 0) { 
	/* split hostList */
	token = strtok(qInfo->hostList, delimiters);
      }
      else {
	token = strtok(NULL, delimiters);
      }
      /*printf("neg 3 tot_hosts, %i,  hosts1[tot_hosts], %s\n",tot_hosts,hosts1[0]);*/

      if(token == NULL)
	break;
      
      numhosts = 1;
      if (strchr(token, '/')) { 
	/* if host is a hostgroup, send it through lsb_hostinfo */
	numhosts = 1;
	token[strlen(token)-1]='\0';
	hostinfo2 = lsb_hostinfo(&token, &numhosts);
	for (i=0;i<numhosts;i++) {

	  hosts1[tot_hosts] = hostinfo2[i].host;

	  /* this is the most embarrassing thing I've ever done, but without this
	   * calls to lsb_hostinfo write over hosts1[0], causing strange behavior */
	  if(tot_hosts == 0) {
	    strcpy(saveme, hostinfo2[0].host);}
	  else {
	    strcpy( hosts1[0], saveme);}

	  tot_hosts++;

	}
      }
      else {
	if(!strcmp(token,"all")) {
	  hosts1[0] = "\0";
	  break;}
	else
	  { 
            token[strcspn(token,"+")] = '\0';
	    hosts1[tot_hosts] = token;
	    tot_hosts++;}
      }

    }

    numhosts = tot_hosts;

    hInfo = lsb_hostinfo(hosts1, &numhosts);
    printf("\n\n Queue Summary:");

  }

  for (i=0;i<numhosts;i++) {


    if (hInfo[i].hStatus & HOST_STAT_UNLICENSED)
      {}/* unlicensed */
    else if (hInfo[i].hStatus & HOST_STAT_UNAVAIL)
      {}/* LIM and sbatchd are unavailable */
    else if (hInfo[i].hStatus & HOST_STAT_UNREACH)
      {}/* sbatchd is unreachable */
    else if (hInfo[i].hStatus & HOST_CLOSED_BY_ADMIN)
      {}/* host closed by administrator */
    else if (hInfo[i].hStatus & ( HOST_STAT_WIND | 
				  HOST_STAT_DISABLED |
				  HOST_STAT_NO_LIM)) 
      {}
    else if (hInfo[i].hStatus & ( HOST_STAT_BUSY | HOST_STAT_FULL | HOST_STAT_LOCKED )) {
      
      if ( strncmp("hero",hInfo[i].host,4) == 0 &		\
	   (strncmp("heroint",hInfo[i].host,7) != 0) &	\ 
	   (strncmp("heroatlas",hInfo[i].host,9) != 0) & \ 
	   (strncmp("herophysics",hInfo[i].host,11) != 0) ) { 	  

	
	strncpy(buffer,hInfo[i].host+4,4);
	/*	printf("hero: %s %s\n", hInfo[i].host, buffer);*/
	if(atoi(buffer) > 3300) {
	  pp_full_hosts++;
	  pp_num_hosts++;
	  pp_num_cores = pp_num_cores + hInfo[i].maxJobs;
	  pp_run_cores = pp_run_cores + hInfo[i].maxJobs;
	  num_cores = num_cores + hInfo[i].maxJobs;
	  run_cores = run_cores + hInfo[i].maxJobs;
	  num_hosts++;}
	else {
	  hero_full_hosts++;
	  hero_num_hosts++;
	  hero_num_cores = hero_num_cores + hInfo[i].maxJobs;
	  hero_run_cores = hero_run_cores + hInfo[i].maxJobs;
	  num_cores = num_cores + hInfo[i].maxJobs;
	  run_cores = run_cores + hInfo[i].maxJobs;
	  num_hosts++;}
      }
      if ( strncmp("iliad",hInfo[i].host,5) == 0 ) {
	iliad_full_hosts++;
	iliad_num_hosts++;
	iliad_num_cores = iliad_num_cores + hInfo[i].maxJobs;
	iliad_run_cores = iliad_run_cores + hInfo[i].maxJobs;
	num_cores = num_cores + hInfo[i].maxJobs;
	run_cores = run_cores + hInfo[i].maxJobs;
	num_hosts++;}
    }
    
    else{ /* the partially used hosts */
      
      t = hInfo[i].numRUN + hInfo[i].numSSUSP + hInfo[i].numUSUSP;
      
      if( t == 0 ) {
	if ( strncmp("hero",hInfo[i].host,4) == 0 &			\
	     (strncmp("heroint",hInfo[i].host,7) != 0) &	\ 
	     (strncmp("heroatlas",hInfo[i].host,9) != 0) & \ 
	     (strncmp("herophysics",hInfo[i].host,11) != 0) ) { 
	  strncpy(buffer,hInfo[i].host+4,4);
	  if(atoi(buffer) > 3300) {
	    pp_empty_hosts++; 
	    pp_num_hosts++; 
	    pp_num_cores = pp_num_cores + hInfo[i].maxJobs;
	    pp_tot_empty_cores = pp_tot_empty_cores + hInfo[i].maxJobs;
	    num_cores = num_cores + hInfo[i].maxJobs;
	    num_hosts++;}
	  else {
	    hero_empty_hosts++; 
	    hero_num_hosts++; 
	    hero_num_cores = hero_num_cores + hInfo[i].maxJobs;
	    hero_tot_empty_cores = hero_tot_empty_cores + hInfo[i].maxJobs;
	    num_cores = num_cores + hInfo[i].maxJobs;
	    num_hosts++;}
	}
	if ( strncmp("iliad",hInfo[i].host,5) == 0 ) {
	  iliad_empty_hosts++; 
	  iliad_tot_empty_cores = iliad_tot_empty_cores + hInfo[i].maxJobs;
	  iliad_num_hosts++; 
	  iliad_num_cores = iliad_num_cores + hInfo[i].maxJobs;
	  num_cores = num_cores + hInfo[i].maxJobs;
	  num_hosts++;}
      }
      else {
	if ( strncmp("hero",hInfo[i].host,4) == 0  &			\
	     (strncmp("heroint",hInfo[i].host,7) != 0) &		\
	     (strncmp("heroatlas",hInfo[i].host,9) != 0) & \ 
	     (strncmp("herophysics",hInfo[i].host,11) != 0) ) {
	  strncpy(buffer,hInfo[i].host+4,4);
	  if (atoi(buffer) > 3300) {
	    pp_empty_cores = pp_empty_cores + ( hInfo[i].maxJobs - t);
	    pp_part_hosts++;
	    pp_num_hosts++;
	    pp_num_cores = pp_num_cores + hInfo[i].maxJobs;
	    pp_tot_empty_cores = pp_tot_empty_cores + ( hInfo[i].maxJobs - t);
	    pp_part_cores = pp_part_cores + t;
	    pp_run_cores = pp_run_cores + t;
	    num_cores = num_cores + hInfo[i].maxJobs;
	    num_hosts++;
	    run_cores = run_cores + t;}
	  else {
	    hero_empty_cores = hero_empty_cores + ( hInfo[i].maxJobs - t);
	    hero_part_hosts++;
	    hero_num_hosts++;
	    hero_num_cores = hero_num_cores + hInfo[i].maxJobs;
	    hero_tot_empty_cores = hero_tot_empty_cores + ( hInfo[i].maxJobs - t);
	    hero_part_cores = hero_part_cores + t;
	    hero_run_cores = hero_run_cores + t;
	    num_cores = num_cores + hInfo[i].maxJobs;
	    num_hosts++;
	    run_cores = run_cores + t; }
	}
	
	if ( strncmp("iliad",hInfo[i].host,5) == 0 ) {
	  iliad_empty_cores = iliad_empty_cores + ( hInfo[i].maxJobs - t);
	  iliad_tot_empty_cores = iliad_tot_empty_cores + ( hInfo[i].maxJobs - t);
	  iliad_part_hosts++;
	  iliad_num_hosts++;
	  iliad_num_cores = iliad_num_cores + hInfo[i].maxJobs;
	  iliad_part_cores = iliad_part_cores + t;
	  iliad_run_cores = iliad_run_cores + t;
	  num_cores = num_cores + hInfo[i].maxJobs;
	  num_hosts++;
	  run_cores = run_cores + t;}
	}
    }
  }
  
  printf("\n\n");
  /*    printf("%-10s %i Total Nodes: %4i Parallel, %i Serial\n", " ", num_hosts, hero_num_hosts, iliad_num_hosts); */
  printf("%-20s %i Total Nodes; %i Total Cores\n\n", " ", num_hosts, num_cores); 

  if ( pp_num_hosts > 0) {
    printf("%-10s %i Special-Projects Parallel Nodes (%i Cores, %i Running, %i Empty):\n", " ", pp_num_hosts, pp_num_cores, pp_run_cores, pp_tot_empty_cores);
    printf("%-26s %i Nodes Empty\n", " ", pp_empty_hosts);
    printf("%-26s %i Nodes Full\n", " ", pp_full_hosts);
    if(pp_part_hosts > 0) 
      {printf("%-26s %i Nodes with %i Cores Empty\n\n", " ", pp_part_hosts, pp_empty_cores);} 
  }

  if ( hero_num_hosts > 0) {
    printf("%-10s %i Parallel Nodes (%i Cores, %i Running, %i Empty):\n", " ", hero_num_hosts, hero_num_cores, hero_run_cores, hero_tot_empty_cores);
    printf("%-26s %i Nodes Empty\n", " ", hero_empty_hosts);
    printf("%-26s %i Nodes Full\n", " ", hero_full_hosts);
    if(hero_part_hosts > 0) 
      {printf("%-26s %i Nodes with %i Cores Empty\n\n", " ", hero_part_hosts, hero_empty_cores);} 
  }
  
  if (iliad_num_hosts > 0) {
    printf("%-10s %i Serial Nodes (%i Cores, %i Running, %i Empty):\n", " ", iliad_num_hosts, iliad_num_cores, iliad_run_cores, iliad_tot_empty_cores);
    printf("%-26s %i Nodes Empty\n", " ", iliad_empty_hosts);
    printf("%-26s %i Nodes Full\n", " ", iliad_full_hosts);
    if(iliad_part_hosts > 0) 
      {printf("%-26s %i Nodes with %i Cores Empty\n\n", " ", iliad_part_hosts, iliad_empty_cores);} 
  }
  
  if(num_preempt_queues > 0 ) 
    printf("\nRunning cores may be associated with preemptable jobs in queues: %s\n", preempt_queues);
  if(who_flag == 1 ) 
    printf("Queue may be used by the following users: %s\n", userlist);
  
  printf("\n");
  
  /*    printf("%-10s %i Nodes Full:  %4i Parallel, %i Serial\n", " ", full_hosts, hero_full_hosts, iliad_full_hosts);
	printf("%-10s %i Nodes Empty: %4i Parallel, %i Serial\n", " ", empty_hosts, hero_empty_hosts, iliad_empty_hosts); */

   
  if ( queue != NULL ) {
    free (hosts1);
    if (who_flag == 1)    
      free (userlist);
  }
}

/* Prints out information about the jobs running that fit the defined parameters */
void print_section( int options, char *inp_queue, char *user, char *inp_hosts) {

  /* variables for simulating bjobs command */
  /*    char *user = "all"; */        /* match jobs for all users */
  struct jobInfoEnt *job;     /* detailed job info */
  struct queueInfoEnt *que;   /* detailed queue info */
  int tot_jobs;               /* total jobs */
  int more;                   /* number of remaining jobs
				 unread */
  int nodenum;                /* Node number counter */
  int i;
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
  
  
  /* gets the total number of jobs. Exits if failure */

  tot_jobs = lsb_openjobinfo(0, NULL, user, inp_queue, inp_hosts, options);
  
  if (tot_jobs < 0) { 
    printf("No matching jobs found\n");
    return;}

  if (tot_jobs<0){ 
    lsb_perror("lsb_openjobinfo");
    exit(-1); }

  printf("%-12s %-8.8s %-6.6s %-7.7s %-4s  %-14s   %-14s %-14s\n",	\
	 "JOBID", "USER", "STAT", "QUEUE", "CORES/NODES", "TIME REMAINING",		\
	 "SUBMIT TIME", "START TIME");
  for (;;) {
    job = lsb_readjobinfo(&more);   /* get the job details */
    if (job == NULL) {
      lsb_perror("lsb_readjobinfo");
      exit(-1);
    }

    targetqueue=job->submit.queue;

    que = lsb_queueinfo(&targetqueue,&numQueues,NULL,NULL,0);

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
    if (job->status == JOB_STAT_RUN)
      {strcpy(statstr,"RUN");}
    else if(job->status == JOB_STAT_PEND)
      {strcpy(statstr,"PEND");} 
    else if(job->status == JOB_STAT_PSUSP)
      {strcpy(statstr,"PSUSP");} 
    else if(job->status == JOB_STAT_SSUSP)
      {strcpy(statstr,"SSUSP");}
    
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
	
	printf("%-12s %-8.8s %-6.6s %-8.8s %4i%1s%2i%1c       %2i%1s%-2.2i%1s%2.2i%1s%2.2i  %-14s",	\
	       lsb_jobid2str(job->jobId), job->user, statstr, job->submit.queue, \
               job->submit.numProcessors, "/",nodenum,exclus, tdays,":", thrs,":",tmin,":",tsec ,submitstr);

	if (options != PEND_JOB) {
	  printf("%-14s",startstr);}
	
	printf ("\n");
	
        if (!more)
	  /* if there are no remaining jobs undisplayed, exits 
	   */
	  break;
  }
  
  printf("%-i total jobs\n",tot_jobs);
  
}

/* Summarizes aspect of cluster currently being polled for jobs, under development */
void print_summary(char *inp_queue, char *user, char *inp_hosts) {
	struct jobInfoEnt *job;     /* detailed job info */
	struct queueInfoEnt *que;   /* detailed queue info */
	struct hostInfoEnt *hos;    /* detailed host info */

  	int numQueues;	      /* number of Queues to query about */
	int numHosts;	      /* number of Hosts */
	char *hostlist;         /* Array of Hosts */
	char *token;
	char *delimiters = " ";
	char *saveptr1;



	if (inp_queue != NULL) {
		printf("Queue Summary\n");

		/* First let's get information on the queue in question */
		numQueues=1;
		que = lsb_queueinfo(&inp_queue,&numQueues,NULL,NULL,0);

		printf(que->hostList);
		printf("\n");

		for(;;) {

			token = strtok_r(que->hostList,delimiters,&saveptr1);

			if (token == NULL) break;
			

			/* We will first poll the hosts available to this queue and see what is going on */

			hostlist=token;

			numHosts=1;

			/*hos = lsb_hostinfo(&que->hostList, &numHosts);*/
			hos = lsb_hostinfo(&hostlist, &numHosts);

			printf("%4i",numHosts);
			}


		}
	else if (inp_hosts !=NULL) {
		printf("Host Summary\n");
		}
	else if (inp_queue == NULL && inp_hosts == NULL) {
		printf("User Summary\n");
		}

}


/* Main routine. */
int main(int argc, char **argv)
{
  
  int c;
  
  static int who_flag = 0;
  char *user = "all", *queue=NULL, *hosts=NULL;
  int i, def;

  /* Initialize Default Flag */
  def=1;

  while (1)
    {
      static struct option long_options[] =
	{
	  /* These options set a flag. */
	  {"help", no_argument,0, 'h'},
	  {"q",  required_argument, 0, 'q'},
	  {"u",  required_argument, 0, 'u'},
	  {"n",  required_argument, 0, 'n'},
	  {"who",  no_argument, 0, 'w'},
	  {0, 0, 0, 0}
	};
      /* getopt_long stores the option index here. */
      int option_index = 0;
     
      c = getopt_long (argc, argv, "u:q:n:w:help",
		       long_options, &option_index);
     
      /* Detect the end of the options. */
      if (c == -1)
	break;
      
      /* Default Flag set to false, we are actually passing options so the default is not used */
      def=0;

      switch (c)
	{
	case 0:
	  /* If this option set a flag, do nothing else now. */
	  if (long_options[option_index].flag != 0)
	    break;
	  printf ("option %s", long_options[option_index].name);
	  if (optarg)
	    printf (" with arg %s", optarg);
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
	  
	case 'w':
	  who_flag = 1;
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
  if (def)
    user = NULL;
  
  /* initialize LSBLIB  and  get  the  configuration
     environment */
  if (lsb_init(argv[0]) < 0) {
    lsb_perror("simbjobs: lsb_init() failed");
    exit(-1);
  }

  printf("ACTIVE JOBS-------------\n");
  print_section(RUN_JOB, queue, user, hosts);
  printf("\nSUSPENDED JOBS-------------\n");
  print_section(SUSP_JOB, queue, user, hosts);
  printf("\n");
  /*print_summary(queue, user, hosts);*/
  print_nodes(who_flag,queue,hosts);
  printf("\n");
  printf("PENDING JOBS-------------\n");
  print_section(PEND_JOB, queue, user, hosts);
  /* when finished to display the job info, close the
     connection to the mbatchd */
  lsb_closejobinfo();
  
  exit(0);
}
