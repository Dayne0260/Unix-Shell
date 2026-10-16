#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include "sh.h"
#include <glob.h>

//Project made by Bryce Newell & Dayne Carney

// Helper function to check if a command is a built-in command
int is_builtin(char *cmd) {
  //A helper function used to determine if a provided command is something inside our project
  if (!cmd) {
    return 0;
  }
  static const char *builtin[] = { //List of accceptable commands
    "exit", "which", "where", "cd", "pwd", "list",
    "pid", "kill", "prompt", "printenv", "setenv", NULL
  };
  for (int i = 0; builtin[i] != NULL; i++) { //Traverse through the list of provided commands
    if (strcmp(cmd, builtin[i]) == 0) { //Compare the user provided command and the current provided command
      return 1; //Command found
    }
  }
  return 0; //Command not inside of provided list
}

char *which(char *command, struct pathelement *pathlist) {
  /* A command used to locate the executable file corresponding to a command inside our shell enviornment */
  if (!command) { return NULL; } //Ensure command was provided in the function
  char buffer[1024]; //A maximum number of bytes allowed

  if (is_builtin(command)) { //Make sure the user function is a provided function
    return NULL;
  }
//This is a check for to see if the user command is a path, file, or directory
  if (command[0] == '/' || strncmp(command, "./", 2) == 0 || strncmp(command, "../", 3) == 0) {
    if (access(command, X_OK) == 0) { //This command asks if the provided command exists and has the execution permissions
      return strdup(command);//Creates a copy of the command and returns a ponter to it
    }
    return NULL;
  }

  struct pathelement *p = pathlist; //Create a temp pathlist
  while (p != NULL) {//The pathlist was created successfuly
    snprintf(buffer, sizeof(buffer), "%s/%s", p->element, command); //Store 'p->element/command' inside buffer
    if (access(buffer, X_OK) == 0) { //Ensure buffer is acessable and executable
      return strdup(buffer);
    }
    p = p->next; //Move to next path inside pathlist
  }
  return NULL; //Something did not work so exit
}

char *where(char *command, struct pathelement *pathlist) {
  /*Searches every directory in PATH and reports all matching executables */
  if (!command) { return NULL; } //Ensure command is not NULL
  char buffer[1024]; //Set a maximum output
  struct pathelement *p = pathlist; //Create a temp PATH for this function to use

  if (is_builtin(command)) { //Ensure command is built-in
    printf("%s is a shell built-in\n", command);
  }

  while (p != NULL) {//Travers the Entire PATH list
    snprintf(buffer, sizeof(buffer), "%s/%s", p->element, command);// Place 'p->element/command' inside of buffer
    if (access(buffer, X_OK) == 0) {//Check to see if buffer is accessable and is executable
      printf("[%s]\n", buffer); //Print PATH
    }
    p = p->next; //Move to next element inside of PATH
  }
  return NULL;
}

void list(char *dir) {
  /*An implementation of the 'ls' command. Opens a given directory path and reads every file and subdirectory
   *then prints to standarnd output*/
  DIR *directory = opendir(dir); //A copy of user directory input
  if (directory == NULL) { //Ensure the directory exists
    perror(dir); //prints an error message to standard output
    return;
  }
  struct dirent *entry; //A pointer which holds information about an individual directory entry
  while ((entry = readdir(directory)) != NULL) { //A check to make sure entry is not at the end of the directory system
    //readdir() will get the next item inside of the directory
    printf("%s\n", entry->d_name); //Print the name of the directory
  }
  if (closedir(directory) == -1) { //Close directory
    perror("closedir");
  }
} /* list() */

void printenv(char **envp) {
  /*Print all system environment variables to standard output */
  while (*envp != NULL) { // Keep printing until reaching the end of the enviromenal variable array
    printf("%s\n", *envp);
    envp++; //Move to next entry
  }
}

int expand_wildcards(char **raw_args, int raw_count, char **out_args, int max_args) {
  /*Takes user input and seraches for wildcards. The use of glob.h helper is used to search through
   *our filebase and return all files which match the user paramters (* and ? wildcards).
   *A helper fuction free_expanded_args is used to free storage used by this function.
   */
  int out_count = 0;
  //A loop to check through each argument in raw_args
  for (int i = 0; i < raw_count; i++) {
    // Check if argument contains * or ?
    // The function 'strpbrk' takes an argument (raw_args[i]) and retruns if it has set characters (* or ?)
    if (strpbrk(raw_args[i], "*?") != NULL) {
      glob_t globbuf;
      /*glob is used to search through our files given * or ? to find matching files
      *raw_args[i] is the string we want to expand (*.c for example)
      *GLOB_NOCHECK is from glob.h and dictates how glob() behaves when no matches are found
      *Pretty much if no files exist, glob() just retruns nothing
      *Passing NULL tells glob() to ignore errors and continuse searching
      *&globbuf is the memory address of the glob_t struct from above, glob() writes its searches into this structure
      *glob() writes total number of matches found and it sets an array of pointers to each matched file names
      */
      int res = glob(raw_args[i], GLOB_NOCHECK, NULL, &globbuf);
      //glob() was successfuly able to loop through our file system
      if (res == 0) {
        //This loop is set to loop through all found file matches
        for (size_t j = 0; j < globbuf.gl_pathc; j++) {
          //A safty check to make sure we don't go outside the max size of out_count array
          if (out_count < max_args - 1) {
            //Store the file matches we found from glob into out_args
            out_args[out_count++] = strdup(globbuf.gl_pathv[j]);
            //out_count will store a value at out_count then increment the value by one
          }
        }
        //Free the space we used from glob()
        globfree(&globbuf);
      }
      //This only occurs if glob() failed to read our file system
      else {
        if (out_count < max_args - 1) {
          //Take the orgainal input and store it into out_args so we can perserve the orgianl users input
          //it's either this or just ignoring it
          out_args[out_count++] = strdup(raw_args[i]);
          //out_count will store a value at out_count then increment the value by one
        }
      }
    } else {
      // Non-wildcard argument from user
      if (out_count < max_args - 1) {
        //Copy user input into output
        out_args[out_count++] = strdup(raw_args[i]);
      }
    }
  }
  //Set end to NULL for any future functions
  out_args[out_count] = NULL;
  //Return the amount of arguments inside of output
  return out_count;
}

void free_expanded_args(char **out_args, int count) {
  //Free memory allocated by expand_wildcards
  for (int i = 0; i < count; i++) {
    if (out_args[i]) {
      free(out_args[i]);
      out_args[i] = NULL;
    }
  }
}

int sh(int argc, char **argv, char **envp) {
  /*Serves as the core command loop driver for the Unix sheel. Manages complete life cycle
   *of reading user input, parsing it, execuring builtin commands, spawing external programs,
   *and cleaning up dynamic memory
   *
   *Continually prompts the user, prints current directoryn and await input untill the user exits
   *or hits end of execution. Supports all commands provided in README.
   */
  int go = 1; // Controls the while loop, when equal to zero the program ends (Exit)
  char prefix[PROMPTMAX] = " "; //Allows for a 'branding' of the shell
  char cwd[PATH_MAX]; //Stores the path of the current working direcotry
  char line[4096]; //This is where user input is stored and read from (Up to 4096 characters)
  char *args[MAXARGS]; //This is used for user arugments (Such as file names and commands)

  /* load the PATH directories once */
  struct pathelement *pathlst = get_path();

  /* the directory I'm in before the last successful cd */
  char previous[PATH_MAX] = "";

	struct sigaction sa_ignore = {0};
  	sa_ignore.sa_handler = SIG_IGN;
  	sigemptyset(&sa_ignore.sa_mask);

  	sigaction(SIGINT, &sa_ignore, NULL); //Ignore Ctrl + C
  	sigaction(SIGTSTP, &sa_ignore, NULL); //Ignore Ctrl + Z

  while (go) {
    //Get the current working directory, display shell command prompt, and print
    //This gets teh absolute path of the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) { //A safety check to make sure cwd was collected
      perror("getcwd");//Print an error message
      free_path(pathlst);//Free space used and exit
      return 1;
    }
    printf("%s [%s]> ", prefix, cwd); //Print CWD and prefix
    fflush(stdout); //Allows printing without using '\n' since you want users to type on the same line as cwd

    //For reading user's raw input from stdin and handle Ctrl + D and errors
    if (fgets(line, sizeof(line), stdin) == NULL) { //Read user input, store into line, make sure user didn't end session (Ctrl + D)
      if (feof(stdin)) {//Checks to see if NULL was returned from stdin
        clearerr(stdin);//Clear NULL Call
        putchar('\n');//print new line
        continue;//Jump to the beginning of 'while(go)' loop
      }
      perror("fgets");//Checks to see if something strange occurs (An error)
      free_path(pathlst);//Clear used memory
      return 1;//End function
    }

    //This portion is used as an input overflow guard and detects when a user types a command
    //over the allocated amount of characters and then rejects the command
    if (strchr(line, '\n') == NULL) { //Seaches through 'line', if a NULL is returned then 'fgets()' execed the ammount of characters we allowed
      int ch;
      while ((ch = getchar()) != '\n' && ch != EOF) { //This is for cleanup if the user input was over 4096 chaceters so nothing is either overwritten or for segumentation faults
        /* reads until the rest of the line is gone */
      }
      if (feof(stdin)) {//Check for an exit event
        clearerr(stdin);//Reset errors flags for stdin
      }
      fprintf(stderr, "Input line too long\n"); //Print error to terminal
      continue;//Jump to top of go()
    }

    // Take raw user input (A jumbled mess of chacters) and splits them into inidividual words seperated by spaces and enforces argment limits
    char *raw_args[MAXARGS]; //An array of string pointers holding parsed words
    int raw_count = 0; //How many raw inputs we have
    char *word = strtok(line, " \t\n");//Scan line for spaces, tabs, and new lines and returns a pointer of a 'word'

    while (word != NULL && raw_count < MAXARGS - 1) { //Make sure the word we found isn't nothing and make sure we haven't exceed the argument limit
      raw_args[raw_count] = word;//Take the 'word' we found in line and store it into our raw_arguments array
      raw_count++; //Update our 'word' count
      word = strtok(NULL, " \t\n"); //Move onto the next 'word' inside 'line'
    }
    raw_args[raw_count] = NULL; //Set the last value inside of our aruguments to NULL (Allows us to traverse or it's a '\0')

    if (word != NULL) {//A user execeed the arugment limit
      fprintf(stderr, "Too many arguments\n");
      continue;//Go back to start of go()
    }
    if (raw_count == 0) {//The user didn't put any arguments (commands)
      continue;//Go back to the start of go()
    }

    int count = expand_wildcards(raw_args, raw_count, args, MAXARGS); //This is how many arguments are in the array after expanding any wildcards in the user input

    //This is executes any commands the user may need/want
    //Check the first word on the command line -> (strcmp(args[0], "command")
    if (strcmp(args[0], "exit") == 0) {
      printf("Executing built-in [exit]\n");
      go = 0;//End Unix Shell execution
    }
    else if (strcmp(args[0], "pwd") == 0) {
      printf("Executing built-in [pwd]\n");
      printf("%s\n", cwd);//Print the current working directory
    }
    else if (strcmp(args[0], "list") == 0) {
      printf("Executing built-in [list]\n");
      if (count == 1) { //There are no target directories or files
        list("."); //List is going to list the contents of the cwd
      } else {
        for (int i = 1; i < count; i++) { //Go through every additional argument typed after list
          printf("\n%s:\n", args[i]); //Print directory name and print what's in args[i]
          list(args[i]); //Call 'list()' on each directory
        }
      }
    }
    else if (strcmp(args[0], "which") == 0) {
      printf("Executing built-in [which]\n");
      if (count == 1) { //The user didn't supply any target commands to lookup
        fprintf(stderr, "which: Too few arguments.\n"); //stderr supplies errors and skips
      } else {
        for (int i = 1; i < count; i++) { //Loop through every command argument after 'which()'
          if (is_builtin(args[i])) { //Checks to see if the argument supplied is an allowed command
            printf("%s: shell built-in command\n", args[i]);
          } else { //The argument supplied is a path
            char *pthname = which(args[i], pathlst); //Search through our linked list of 'PATH' to locate teh first match executable file for args[i]
            if (pthname != NULL) { //A safty check
              printf("[%s]\n", pthname); //Print absolute path to executable
              free(pthname); //Free used memory
            } else { //Argument wasn't found as a command or path
              fprintf(stderr, "%s: Command not found\n", args[i]);
            }
          }
        }
      }
    }
    else if (strcmp(args[0], "where") == 0) {
      printf("Executing built-in [where]\n");
      if (count == 1) { //'Where' was the first command since bootup
        fprintf(stderr, "where: Too few arguments\n");
      } else {
        for (int i = 1; i < count; i++) { //Loop through all of our arguments
          where(args[i], pathlst); //Go through the entire path in every directory
        }
      }
    }
    else if (strcmp(args[0], "cd") == 0) {
      printf("Executing built-in [cd]\n");

      if (count > 2) { //The user tried to move into multiple directories
        fprintf(stderr, "cd: Too many arguments\n");
      } else {
        const char *destin = NULL; //The directory we want to go to
        if (count == 1) { //If the user only enters 'cd' it gets the users home directory
          destin = getenv("HOME"); //Set our directory to the home directory
          if (destin == NULL || *destin == '\0') { //Check for errors in destination such as the no set home or an incorrect directory set
            fprintf(stderr, "cd: HOME is not set\n");
          }
        } else if (strcmp(args[1], "-") == 0) { //Move to the previous directory
          if (previous[0] == '\0') { //This is for when you're in the root direcotry and there are no directories before the one you're inside
            fprintf(stderr, "cd: No previous directory\n");
          } else {
            destin = previous; //Move the the previous directory
          }
        } else {
          destin = args[1]; //Move the the user defined directory
        }

        if (destin != NULL) {
          if (chdir(destin) == -1) { //For directories which the user can't access for any reason
            perror("cd"); //Throw an error and move on
          } else {
            strcpy(previous, cwd); //Update cwd to the previous directory (The directory we want to move into)
          }
        }
      }
    }
    else if (strcmp(args[0], "pid") == 0) {
      printf("Executing built-in [pid]\n");
      printf("%ld\n", (long)getpid()); //Get the process ID of the current running process
    }
    else if (strcmp(args[0], "kill") == 0) {
      printf("Executing built-in [kill]\n");

      if (count != 2 && count != 3) { // Make sure there's only 2 or less proccesses occuring
        fprintf(stderr, "Usage: kill [-signal] pid\n");
      } else {
        int signalnum = SIGTERM;
        char *end;
        const char *pid_txt = args[1];
        int valid = 1;

        if (count == 3) {
          if (args[1][0] != '-' || args[1][1] == '\0') { //Make argument sure is a proccess
            fprintf(stderr, "kill: Invalid signal\n");
            valid = 0;
          } else {
            errno = 0;
            long num = strtol(args[1] + 1, &end, 10);
            if (errno != 0 || end == args[1] + 1 || *end != '\0' || num <= 0 || num > INT_MAX) { //Make sure argument is valid to kill
              fprintf(stderr, "kill: Invalid signal\n");
              valid = 0;
            } else { //pid_txt is set the the process we want to kill
              signalnum = (int)num;
              pid_txt = args[2];
            }
          }
        }

        if (valid) { //If this is a valid process to kill
          errno = 0;
          long num = strtol(pid_txt, &end, 10); //Convert our process to to an PID

          if (errno != 0 || end == pid_txt || *end != '\0' || num <= 0 || num > INT_MAX) { //This is a safty check for over/underflow, empty strings, incorrect arguments, and invalid PID's
            fprintf(stderr, "kill: Invalid PID\n");
          } else if (kill((pid_t)num, signalnum) == -1) { //Ensure targeted process is correct or exists
            perror("kill"); //kill process
          }
        }
      }
    }
    else if (strcmp(args[0], "prompt") == 0) {
      printf("Executing built-in [prompt]\n");
      if (count > 2) { //A user argument check
        fprintf(stderr, "prompt: Too many arguments\n");
      } else if (count == 2) { //User argument check
        if (strlen(args[1]) >= sizeof(prefix)) {
          fprintf(stderr, "prompt: Prefix too long\n");
        } else { //Change prefix to what's in 'args[1]'
          strcpy(prefix, args[1]);
        }
      } else { //When users just type 'prompt' ie one argument
        char newprefix[4096];
        printf(" input prompt prefix:");
        fflush(stdout); //Appear without needing a new line

        if (fgets(newprefix, sizeof(newprefix), stdin) == NULL) { //Read response and make sure user didn't exit or want to return
          if (feof(stdin)) {
            clearerr(stdin); //Clear errors from stdin
            putchar('\n');
          } else {
            perror("fgets"); //Read errors
          }
        } else {
          if (strchr(newprefix, '\n') == NULL) { //Makes sure the user didn't put something in longer than 4096 chacters
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {} //clean up the string (replace \n with '\0')
            if (feof(stdin)) {
              clearerr(stdin); //Clear errors
            }
            fprintf(stderr, "prompt: Prefix too long\n");
          } else {
            newprefix[strcspn(newprefix, "\n")] = '\0';
            if (strlen(newprefix) >= sizeof(prefix)) { // A safty check to make sure prefix isn't too long
              fprintf(stderr, "prompt: Prefix too long\n");
            } else {
              strcpy(prefix, newprefix); //Set new prefix
            }
          }
        }
      }
    }
    else if (strcmp(args[0], "printenv") == 0) {
      printf("Executing built-in [printenv]\n");
      if (count == 1) { //Only 'printenv' was in arguments
        extern char **environ; //A pointer array which contains all key values environment pairs
        printenv(environ);
      } else if (count == 2) { //An argument was passed with 'printenv'
        char *value = getenv(args[1]); //Look up what the user put as an argument
        if (value != NULL) { // The user input exists print the value of the input
          printf("%s\n", value);
        }
      } else { //Rejects more than alloted arguments (more than 1 plus 'printenv')
        fprintf(stderr, "printenv: Too many arguments.\n");
      }
    }
    else if (strcmp(args[0], "setenv") == 0) {
      printf("Executing built-in [setenv]\n");

      if (count == 1) { //One user argument
        extern char **environ; //A pointer array which contains all key values environment pairs
        printenv(environ);
      } else if (count > 3) { //Too many user arguments
        fprintf(stderr, "setenv: Too many arguments.\n");
      } else {
        const char *value = (count == 3) ? args[2] : ""; //If the user provide a value, valuse is set otherwise it leaves the value blank

        if (setenv(args[1], value, 1) == -1) { //Creates or modifies value with args[1]
          perror("setenv");
        } else if (strcmp(args[1], "PATH") == 0) { //Update path
          free_path(pathlst);
          pathlst = get_path();
        }
      }
    }
    /* External Program Execution */
    else {
      char *pthname = NULL;
      if (strchr(args[0], '/') != NULL) { //Check to see if arguemnt is a path type
        struct stat info;

        if (access(args[0], X_OK) == 0 && stat(args[0], &info) == 0 && S_ISREG(info.st_mode)) { //Check and see if this is a PATH or directory
          pthname = strdup(args[0]);
        }
      } else { //It's a path
        pthname = which(args[0], pathlst); //Make an extra copy of the path
      }

      if (pthname != NULL) { //This is fork() so check to see if we have a duplicate path
        printf("Executing [%s]\n", pthname);
        fflush(stdout);

        pid_t child = fork(); //Duplicate the shell proocess into a child
        if (child == -1) { //Fork() returned with an error
          perror("fork");
          free(pthname);
        } else if (child == 0) { //Fork(() returned
          extern char **environ; //A pointer array which contains all key values environment pairs
          struct sigaction action = {0}; //Create a struck called action and clear all fields
          action.sa_handler = SIG_DFL; //Tells action what action to take when signal arives (preform OS default action)
          sigemptyset(&action.sa_mask); //Wait for a particaul 'start' sign to start process

          if (sigaction(SIGINT, &action, NULL) == -1 ||
              sigaction(SIGTSTP, &action, NULL) == -1 ||
              sigaction(SIGTERM, &action, NULL) == -1) {
            perror("sigaction");
            _exit(126);
          } // Allows for forground programs to respond to interrupts/stop signals

          execve(pthname, args, environ); //Error handling
          perror(pthname);
          _exit(126);
        } else {
          int status;
          pid_t result;

          do { //Suspends shell untill foreground child process finishes
            result = waitpid(child, &status, 0);
          } while (result == -1 && errno == EINTR);

          if (result == -1) {
            perror("waitpid");
          } else if (WIFEXITED(status)) { // Check tot see if process terminated normal
            int exitstatus = WEXITSTATUS(status); //Get return code
            if (exitstatus != 0) {
              printf("Exit status: %d\n", exitstatus);
            }
          } else if (WIFSIGNALED(status)) {
            printf("Terminated by signal: %d\n", WTERMSIG(status));
          }
          free(pthname); //Free memory used
        }
      } else {
        fprintf(stderr, "%s: Command not found.\n", args[0]);
      }
    }

    /* Free memory allocated by expand_wildcards */
    free_expanded_args(args, count);
  }

  /* Clean up memory on shell exit */
  free_path(pathlst);
  return 0;
} /* sh() */