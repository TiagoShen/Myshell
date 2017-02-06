//COMP 3230A Assignment 2
//Create by:
//Shen Si Yuan
//Platform: Ubuntu 14.04
//Last modification 2016-10-22 1:48 p.m.


#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <math.h>

#define true 1
#define false 0

//Some useful functions declared below

char * getl();	//Get the line of input

void saveChildInfo(double time[], int pid, char *line);	//Save the information of the terminated child process

char * strrep(char *orig, char *rep, char *with);	//String replacement

void sigchldHandler(int signum, siginfo_t *sig, void *v);

double rtime[2];
int showInfo = false;
int background = false;
int waitng = false;
int childNum;
int ct=0;

int main() {

	//Get the home direction of the current user
	char *homedir;
	if((homedir = getenv("HOME"))==NULL){
		printf("myshell: input your password:\n");
		homedir = getpwuid(getuid())->pw_dir;
	}

	//Set to ingnore the interrupt signal
	//signal(SIGINT, SIG_IGN);

	//Some printf-s are used for debug purpose
	//printf("HOME: %s\n", homedir);

	//int count = 0;

//----------------READ INPUT--------------------------------
while(1){
	while(waitng){
		sleep(1);
	}
	waitng = false;
	char *line;
	//count++;

	//printf("EXECUTE THE %d-th LINE.\n", count);
	//printf("PID: %d\n", getpid());
	printf("## myshell $ ");
	//Get input and seperate input into arguments
	line=getl();

	//Not input will continue the next iteration
	if(strlen(line)==0) continue;

	char ** arg = malloc(100*sizeof(char*));
	size_t maxlen = 100;
	//length of the input line seperated by space
	size_t len = 0;
	size_t inputsize = strlen(line); 
	char *inputline = malloc(inputsize*sizeof(char));
 	strcpy(inputline, line);
	char *fileName;
	char *filePath;
	int showChildInfo = false;
	int exeWithPath = false;

	//printf("YOUR INPUT IS %s\n", line);
	//Seperate the input into arguments array
	char *token = strtok(line, " \t");
	while (token != NULL){
		arg[len] = token;
		len++;
		if(len>=maxlen){
			arg=realloc(arg, 2*maxlen*sizeof(char));
		}
		token = strtok (NULL, " \t");
	}

	//Built in command 'exit'
	if(strcmp(arg[0],"exit")==0){
	 if(len>1){
	 	printf("myshell: \"exit\" with other arguments\n");
	 	continue;
	 }
	 else{
	 	exit(0);
	 }
	}else{
		//printf("NO EXIT COMMAND FOUND.\n");
	}

//-------------------Check pipe----------------------
	int pipePos[100];	//pipe character '|' 's postions in the argument array
	int pipeNum=0; //Number of pipes
	int childNo=0; //Used to mark the child process number
	int co;

	//Initialize pipePos to zero
	for(co=0; co<100; co++){
		pipePos[co] = 0;
	}

	//Record position of the pipe and count the number of pipes
	for(co=0; co<len; co++){
		if(strcmp(arg[co],"|") == 0){
			//printf("Pipe at %d\n", co);
			pipePos[pipeNum] = co;
			pipeNum++;
		}
	}

	int hasPipe = false;
	//printf("NUMBER OF PIPES: %d\n", pipeNum);

	//2D Array, pipes
	int pfd[2];
	int **pfds = malloc(pipeNum*sizeof(pfd));
	if(pipeNum>0){
		hasPipe = true;
	}else{
		hasPipe = false;
	}

	//Initialize pipes
	int cnm;
	for(cnm=0; cnm<pipeNum; cnm++){
		pfds[cnm] = malloc(2*sizeof(int));
		pipe(pfds[cnm]);
	}
	pid_t *childPids = malloc((pipeNum+1)*sizeof(pid_t));
	char **childNames = malloc((pipeNum+1)*sizeof(char*));
	int restart = false;	//Use to judge if there's any wrong in the process and continue to another prompt

	showInfo = false;
	background = false;
	if(hasPipe){ childNum = pipeNum+1;}
	else{ childNum = 1; }
//------------------------------PIPEING CREATE EACH PROCESS IN EACH ITERATION----------------------------------------
for(childNo = 0; childNo<pipeNum+1; childNo++){

	int beg, en;					//Positions of its own arguments appeared in the arg array: argp = arg[beg~end]
	int isBeg=false;				//Is the beginning process
	int isEn=false;					//Is the end process

	if(hasPipe==false){
		isBeg = true;
		isEn = true;
	}
	//Determine the positions of the process arguments
	if(childNo == 0){				//BEGINNING CHILD
		isBeg = true;
		beg = 0;
		en = pipePos[0]-1;
	}else if(childNo == pipeNum && childNo != 0){	//END CHILD
		isEn = true;
		beg = pipePos[childNo-1]+1;
		en = len-1;
	}else{
		beg = pipePos[childNo-1]+1;
		en = pipePos[childNo]-1;
	}
	//printf("CHILDNO: %d, BEGIN: %d, END: %d\n", childNo, beg, en);

	//Copy its own arguments and store in argp
	int numOfArg = en-beg+1;
	if(hasPipe == false){ 
		numOfArg = len;
		beg = 0;
		en = 0;
	}
	//printf("number of arguments: %d\n", numOfArg);
	char **argp = malloc(numOfArg*sizeof(char*));
	int ct;
	for(ct = 0; ct<numOfArg; ct++){
		argp[ct] = malloc(strlen(arg[ct])*sizeof(char));
		strcpy(argp[ct],arg[beg+ct]);
		//printf("CT: %d\n", ct);
	}

//-------------------------CREATE CHILD PROCESS-----------------------------------
	//Built in command timeX
	//printf("Number of arguments: %d\n", numOfArg);
	if(strcmp(argp[0], "timeX")==0 && isBeg){
		if(numOfArg==1){
			printf("myshell: \"timeX cannot be standalone command\"\n");
			restart = true;
			break;
		}	
		int a=0;
		for(a=0; a<numOfArg-1; a++){
			argp[a]=argp[a+1];		
		}
		numOfArg--;
		showInfo = true;
		//printf("Built in command timeX.\n");
	}
	
	//Check whether its background mode
	if(strcmp(argp[numOfArg-1], "&")==0 || (argp[numOfArg-1])[strlen(argp[numOfArg-1])-1] == '&'){
		if(isEn){
			background = true;
			numOfArg--;
		}else{
			printf("myshell: '&' should not appear in the middle of the command line\n");
			restart = true;
			break;
		}
		//printf("Background Mode.\n");
	}else{
		background = false;
	}

	//Check file path
	if((argp[0])[0] == '.' && (argp[0])[1] == '/'){
		//printf("RELATIVE PATH: %s\n", argp[0]);
		exeWithPath = true;
	}else if((argp[0])[0] == '~' && (argp[0])[1] == '/'){
		argp[0] = strrep(argp[0],"~",homedir);
		//printf("PATH: %s\n",argp[0]);
		exeWithPath = true;
	}else if((argp[0])[0] == '/'){
		//printf("PATH: %s\n",argp[0]);
		exeWithPath = true;
	}else{
		exeWithPath = false;
	}
	//printf("FILE PATH: %s\n", filePath);
	//Create argument array argv, which will be used in execvp() function
	//printf("THERE ARE %d ARGUMENTS\n", numOfArg);
	char **argv = malloc(numOfArg*sizeof(char*));
	int l=0;
	for(l=0; l<numOfArg; l++){
		argv[l] = argp[l];
	}

	childNames[childNo] = malloc(strlen(argp[0]+1)*sizeof(char));
	strcpy(childNames[childNo], argp[0]); 
	filePath = malloc(strlen(argp[0]+1)*sizeof(char));
	strcpy(filePath, argp[0]);
	filePath[strlen(argp[0])]='\0';
	//printf("FILE PATH LENGTH: %d\n", strlen(filePath));
	//printf("FILE PATH: %s\n", filePath);
	if(exeWithPath){
		token = strtok(argp[0], "/");
		while (token != NULL){
			fileName = token;
			token = strtok (NULL, "/");
		}
		
	}
	if(exeWithPath){
		argv[0] = fileName;
	}
	//printf("FILE NAME: %s\n", fileName);


	struct sigaction sa;
	sigaction(SIGCHLD, NULL, &sa);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigchldHandler;
	sigaction(SIGCHLD, &sa, NULL);
//----------------------------Create and start child processes-------------------------
	pid_t pid = fork();
	struct timeval tv;
	gettimeofday(&tv, NULL);
	rtime[0] = (double)tv.tv_sec+((double)tv.tv_usec)/1000000;
	if (pid < 0) {
		printf("fork: error no = %s\n", strerror(errno));
		exit(-1);
	} else if (pid == 0) {							//Child process
		//int qnm;
		//for(qnm = 0; qnm<numOfArg; qnm++)
			//printf("argv: %s\n", argv[qnm]);
		//printf("child: I am a child process, with pid: %d\n", (int) getpid());

	//----------Pipeing----------------------
		if(isBeg && hasPipe){						//BEGINNING 
			int qnm;
			for(qnm = 1; qnm <pipeNum; qnm++){
				close((pfds[qnm])[0]);
				close((pfds[qnm])[1]);
			}
			close((pfds[0])[0]);
			dup2((pfds[0])[1], 1);
		}else if(isEn && hasPipe){					//END
			int qnm;
			for(qnm = 0; qnm <pipeNum-1; qnm++){
				close((pfds[qnm])[0]);
				close((pfds[qnm])[1]);
			}
			close((pfds[pipeNum-1])[1]);
			dup2((pfds[pipeNum-1])[0], 0);
		}else if(hasPipe){							//MIDDLE
			int qnm;
			int left, right;
			left = childNo;
			right = pipeNum-left;
			for(qnm = 0; qnm <left-1; qnm++){
				close((pfds[qnm])[0]);
				close((pfds[qnm])[1]);
			}
			close((pfds[left-1])[1]);
			dup2((pfds[left-1])[0], 0);
			for(qnm =childNo+1; qnm<childNo+right; qnm++){
				close((pfds[qnm])[0]);
				close((pfds[qnm])[1]);
			}
			close((pfds[childNo])[0]);
			dup2((pfds[childNo])[1], 1);
		}

		//Set signal actions to default behaviors
		signal(SIGINT, SIG_DFL);
		//printf("argv[0]: %s\n", argv[0]);
		if(execvp(filePath,argv) == -1){
			printf("myshell: %s: %s\n",filePath, strerror(errno));
			exit(errno);
			restart = true;
			break;		
		}
	} else {			//Parent process
		//printf("I am a parent process, with pid: %d and my child's pid is %d\n", (int) getpid(), (int) pid);

		if(hasPipe){
			childPids[childNo] = pid;
		}
	}
}//END OF FOR-LOOP OF PIPE

//---------------------------Creation and start of child processes end---------------------------------

	//If something wrong with the for-loop, continue to another prompt
	if(restart) continue;

	//Parent close all pipes
	if(hasPipe){
		int cn;
		for(cn = 0; cn < pipeNum; cn++){
			close((pfds[cn])[0]);
			close((pfds[cn])[1]);
		}
		
	}
	siginfo_t info;
	waitid(P_ALL, WNOWAIT, &info, 0);
	if(!background){
		waitng = true;
	}


}//END OF WHILE-LOOP

	return 0;
}

//Get the line of input
char * getl(){
	char * line = malloc(100), * linep = line;
	size_t lenmax = 100, len = lenmax;
	int c;
	
	if(line == NULL)	return NULL;
	for(;;){
		c = fgetc(stdin);
		if(c == EOF) break;
		if(--len == 0){
			len = lenmax;
			char * linen = realloc(linep, lenmax *=2);

			if(linen == NULL){
				free(linep);
				return NULL;
			}
			line = linen + (line - linep);
			linep = linen;
		}
		
		if((*line++ = c) == '\n')
			break;
	}
	*line = '\0';
	size_t length = strlen(linep);
	linep[length-1] = '\0';
	return linep;
}


// String replacement
char *strrep(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}


//Child handler
void sigchldHandler(int signum, siginfo_t *sig, void *v) {
	char str[50];
	FILE *file;
	int z;
	char ss[50];
	char sq[50];
	double ti[2];
	ti[1]=0;
	ti[0]=0;
	unsigned long h, ut, st;
	sprintf(str, "/proc/%d/stat", (int)sig->si_pid);
	file = fopen(str, "r");
	if (file == NULL) {
		printf("Error in open my proc file\n");
	}
	fscanf(file, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu", &z, sq, &ss[0], &z, &z, &z, &z, &z, 
		(unsigned *)&z, &h, &h, &h, &h, &ut, &st);
	fclose(file);
	if(showInfo){
		//printf("The number of clock ticks per second is: %ld\n", sysconf(_SC_CLK_TCK));
		ti[1]=st*1.0f/sysconf(_SC_CLK_TCK);
		ti[0]=ut*1.0f/sysconf(_SC_CLK_TCK);
		struct timeval tv;
		gettimeofday(&tv, NULL);
		rtime[1] = (double)tv.tv_sec+((double)tv.tv_usec)/1000000;
		double rt = rtime[1]-rtime[0];
		printf("PID:     CMD:                RTIME:              UTIME:              STIME:              \n");
		printf("%-9d%-20s%-20.2lf%-20.2lf%-20.2lf\n", sig->si_pid, sq, rt, ti[0], ti[1]);
	}
	if(background) printf("[%d] %s Done\n",sig->si_pid, str);
	waitpid(sig->si_pid,NULL,0);
	ct++;
	//printf("CT: %d, childNum: %d\n", ct, childNum);
	if(ct == childNum){ 
		waitng = false;
		ct = 0;
	}
}