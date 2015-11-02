#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define history_file_path  "/tmp/Shell_History.txt" //History file path
#define log_file_path "/tmp/Shell_Log.txt" //Log file path
#define max_line_length 515 // maximum size of command line.
#define args_length 100 // maximum sieze of arguments array.
#define max_var_number 1000 //maximum number of allowed variables

char * pathEnv; //path contains all possible command pathes
FILE *hf, *bf, *lf; //history , batch and log files pointers
char line[ max_line_length ]; //input
char args[args_length][max_line_length]; //2D array containg all input as array of strings spliteed on space and ' "
char var[max_var_number][max_line_length]; //2D array containing all variable declared durin gruntime
char value[max_var_number][max_line_length]; //Maping to the variables, containing values of the variables
int argsNum; //argument number + command
int varIdx; //number of current declared variables


//clear argument for using in the next input
void clearArgs(){
    int i,j;
    for( i = 0; i < args_length; ++i){
        line[i] = 0;
        for(j = 0; j < args_length; ++j){
            args[i][j] = 0;
        }
    }

}


//splitting on space the input line
int splitOnSpace( char line[], int argIdx )
{
	char temp_line[ strlen( line ) ];
	strcpy( temp_line, line );

	char * token = strtok( temp_line, " " );
    if(token==NULL) {
        args[ argIdx ][0] = '\0';
    }
	while( token != NULL )
	{
		strcpy( args[ argIdx++ ], token );
		token = strtok( NULL, " " );
	}

	return argIdx;

}


//parsing the input user entred
//by first splitting in ' or " then splitting each block on space
void parseLine( char * line )
{
	int size = strlen( line ), argIdx = 0;
	char quote = ' ';   // no quotes
    line[size-1]='"';
    int i=0;
	for( i = 0; i < size; i++ )
	{
		if( line[i] != '\'' && line[i] != '"' )
		{
			char tmp[2] = { line[i] };
			strcat( args[ argIdx ], tmp );
		}
		else if( quote == ' ' )
		{

			quote = line[i];
			argIdx = splitOnSpace( args[argIdx], argIdx );
		}
		else if( line[i] == quote )
		{
			++argIdx;
			quote = ' ';
		}
		else
		{
			return;
		}

	}

	argsNum = argIdx;
}

//Print all arguments entries for testing
void print_args(){
    printf("args:\n");
    int i=0;
    printf("%d",argsNum);
    for(i=0;i<argsNum;++i){
        printf("%s\n",args[i]);
    }
}

//check if user input contains =
bool lineHasVar(){
    int i;
    for(i = 0;i < strlen(line); ++i){
        if(line[i]=='='){return true;}
    }
    return false;
}

//terminate the shell
void terminate(){

    exit(0);
}

//get the value of the declared variable
char * get_var_value(char  arr[]){
    char temp[max_line_length];
    strcpy(temp,arr);
    if(getenv(temp)!=NULL){
        return getenv(temp); //return if input one of the common path like PATH, HOME
    }else
    {
        int i;
        for(i = varIdx-1;i >= 0; --i){

            if(strcmp(var[i],arr)==0){
                return value[i]; //return if found in the declared variables
            }
        }
        return arr; //return if not one of the declared variables
    }
}



void replace_args_values(){
    int i,j;
    for(i = 0;i < argsNum; ++i){
        if(args[i][0]=='$'){
            char temp_new_arg[max_line_length];
            char temp_checked_var[max_line_length];
            for(j = 1;j < strlen(args[i]);++j){
                temp_checked_var[j-1]=args[i][j];
            }
            temp_checked_var[j-1]='\0';
            strcpy(temp_new_arg,get_var_value(temp_checked_var));
            strcpy(args[i],temp_new_arg);
        }
    }
}

//adding user input to the history file
void addtoHistory(){
    hf = fopen(history_file_path,"a");
    if(hf != NULL){
        fprintf(hf,"%s",line);
    }else{
        perror(""); //file not found
    }
        fclose(hf);
}


//execute the command entered
void execute(char *command){
    replace_args_values(); // replacing all $var to thier values

    //parsing the PATH array to check all possible files for the command enterd
    //when found create process to execute that command
    char * tok,temp[max_line_length];
    char pathTemp[max_line_length];

    bool back=false;

    strcpy(pathTemp,pathEnv);
    tok = strtok (pathTemp,":");
    while (tok != NULL)
    {
        //printf ("%s\n",tok);
        if(command[0]=='/'){
            strcpy(temp,command);
        }else{

            strcpy(temp,tok);
            strcat(temp,"/");
            strcat(temp,command);
        }
        if(access(temp,X_OK)==0){ //found command file, then begin execution
            pid_t pid;
            int status;

            char *argsTemp[argsNum];
            int i;
            for(i = 0;i < argsNum;++i){
                argsTemp[i] = (char *)malloc(max_line_length);
                strcpy(argsTemp[i],args[i]);
            }
            argsTemp[argsNum]=NULL;
            if(strcmp(argsTemp[argsNum-1],"&")==0){
                back=true;
                argsTemp[argsNum-1]=NULL;
            }
            pid = fork(); //creating new process
            if(pid == 0){ //child process
                if(execv(temp,argsTemp) == -1){
                    perror("");
                    return;
                }else{
                    return;
                }
            }else{  //parent process
                if(!back){//Foreground process
                    waitpid(pid, &status, 0); //waiting for the process to terminate
                    return;
                }
                //background process
                return;
            }
        }
        tok = strtok (NULL, ":"); //continue splitting
        memset(temp,0,max_line_length);
    }
    //if command not found
    if(lineHasVar()){ //if there is = then a variable declaration detected and add it to the shell memmory
        int i,j=0,flag=0;
        for(i = 0;i < strlen(line)-1; ++i){
            if(line[i]!='=' && flag==0){
                var[varIdx][i]=line[i];

            }else if(line[i]=='=' && flag==0){
                flag=1;
            }else{
                value[varIdx][j]=line[i];
                ++j;
            }
        }
        ++varIdx;
    }
    if(!lineHasVar()){
        printf("Command Not Found\n");
    }
    return ;
}



//Runing the shell
void run(){

    addtoHistory(); //add input user to the history file
    parseLine(line); //parsing the input using parser
    if(args[0][1]=='#'){
        return ; //comment ignore the whole line
    }
    if(strcmp(args[0],"exit")==0){
        terminate();
    }else if(strcmp(args[0],"cd")==0){
        if(args[1]==NULL){
            chdir(getenv("HOME"));
            return;
        }else if(strcmp(args[1],"~")==0){
            chdir(getenv("HOME"));
            return;
        }else{
            if(chdir(args[1])==0){
                return;
            }else{

                perror(""); //Error Dierctory not found
                return;
            }
        }
    }else if(strcmp(args[0],"history")==0){
        char tempLine[max_line_length];
        hf = fopen(history_file_path,"r");
        if(hf!=NULL){
            while(fgets(tempLine,max_line_length,hf)!=NULL){
                fputs(tempLine,stdout);
            }
        }else{
            perror(""); //Print error message when file not found
        }
        fclose(hf);
        return;
    }
    else{// Execute all remaining code like Cp, rm ,gedit....etc
        execute(args[0]);
    }


}


//Logging the process terminations creaetd by system calls
void toLog(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    lf = fopen(log_file_path,"a");
    if(lf != NULL){
        fprintf(lf,"%d-%d-%d %d:%d:%d Process Finished\n",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    fclose(lf);
}

//some intialization for the Shell program
void intialize(){
    pathEnv = getenv("PATH"); //getting all valid path containging all commands pathses
    varIdx = 0; //setting number of current declared variable to zero
    chdir(getenv("HOME")); //setting current directory work to the machine HOME
}


//main function
int main(int argc, char **argv){

    signal(SIGCHLD,toLog);//chiled process termination aleret
    intialize(); //setting intializing for the Shell program
    bf = fopen(argv[1],"r");
    if(bf != NULL){
        while(fgets(line,max_line_length,bf)!= NULL){
            if(line[0]=='\n'||line[0]=='\r')terminate();
            run();
            clearArgs();
        }
    }
   else{
            perror(""); // No File found, continue interactive mode
            printf("Shell>");
            while(fgets(line,max_line_length,stdin)!=NULL){
                if(line[0]=='\n'||line[0]=='\r'){
                    clearArgs();
                    printf("Shell>");
                    continue;
                }
                run();
                clearArgs();
                printf("Shell> ");
            }
            return 0;
    }

    return 0;
}
