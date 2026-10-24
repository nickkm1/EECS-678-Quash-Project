/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>

#include "quash.h"

#include <fcntl.h>

#include <unistd.h>

#include "deque.h"

#include <limits.h>

#include <signal.h>

// struct for job and info around the job
typedef struct Job {
  // job id
    int job_id;
    // process id
    pid_t* pids;
    // number of processes
    int pid_count;
    // inputted command string
    char* cmd;
} Job;

IMPLEMENT_DEQUE_STRUCT(JobQueue, Job);
IMPLEMENT_DEQUE(JobQueue, Job);

static JobQueue background_jobs;
static bool job_queue_initialized = false;

/**
 * @brief Helper to add a background job to the job queue
 */
static void add_job(int job_id, pid_t* pids, int num_pids, char* cmd) {
    // ensure there is at least one valid process ID associated with the job
    if (num_pids <= 0) {
        return;
    }

    // initialize the background job queue lazily if it hasn't been initialized yet
    if (!job_queue_initialized) {
        background_jobs = new_JobQueue(10);
        job_queue_initialized = true;
    }

    Job job;

   
    job.job_id = job_id;
    job.pid_count = num_pids;
    job.cmd = cmd;

    // allocate memory to store all process IDs belonging to this job
    job.pids = malloc(sizeof(pid_t) * num_pids);
    if (job.pids == NULL) {
        perror("malloc");
        return;
    }

    // copy over each process ID into the job's allocated pid array
    for (int i = 0; i < num_pids; i++) {
        job.pids[i] = pids[i];
    }

    // push newly created job onto the back of the background job queue
    push_back_JobQueue(&background_jobs, job);
}

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// IMPLEMENTED: functions on lines 26-43, 47-61, 179-191
// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {

  // get the path and allocate memory for it
  char* cwd = malloc(PATH_MAX);

  // if there is no path then 
  if (cwd == NULL) {
    // false because no memory was actually used
    *should_free = false;
    // return an empty string
    return "";
  }

  // store the path inside of cwd, if it fails then go into the if statement
  if (getwd(cwd) == NULL) {
    // free the memory allocated to it
    free(cwd);
    // because we just freed it
    *should_free = false;
    // return empty string
    return "";
  }

  // should actually be freed because there's something 
  *should_free = true;
  // return the cwd
  return cwd;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple
  const char* val = getenv(env_var);

  // if getenv returns NULL, return empty string to avoid crash
  if (val == NULL) {
    return "";
  }
  
  return val;
}

// Check the status of background jobs
void check_jobs_bg_status() {
  if (!job_queue_initialized)
    return;

  int length = length_JobQueue(&background_jobs);
  
  // iterate through the job queue
  for (int i = 0; i < length; i++) {
    Job job = peek_front_JobQueue(&background_jobs);
    
    // pop it temporarily to process it, will push it back if it is still running
    pop_front_JobQueue(&background_jobs);

    bool all_completed = true;

    // check all processes belonging to this job
    for (int p = 0; p < job.pid_count; p++) {
      int status;
      // WNOHANG polls without blocking
      pid_t result = waitpid(job.pids[p], &status, WNOHANG);
      
      if (result == 0) {
        // process is still running
        all_completed = false;
      }
    }


    if (all_completed) {
      // print completion message
      print_job_bg_complete(job.job_id, job.pids[0], job.cmd);
      // free allocated memory for PIDs and command string
      free(job.pids);
      free(job.cmd);
    } else {
      // put it back in the queue if it's still running
      push_back_JobQueue(&background_jobs, job);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;



  // TODO: Remove warning silencers
  // (void) exec; // Silence unused variable warning
  // (void) args; // Silence unused variable warning


  // TODO: Implement run generic
  // IMPLEMENT_ME();
  
  // run execvp, exec= the command, and args = the arguments for the command
  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // loop until null
  for (int i = 0; str[i] != NULL; i++) {
    // add a space between all except the first one
        if (i > 0) {
            printf(" ");
        }
        // print the array at the index of i
        printf("%s", str[i]);
    }
  // new line
  printf("\n");  

  // TODO: Remove warning silencers
  // (void) str; // Silence unused variable warning

  // TODO: Implement echo
  // IMPLEMENT_ME();

  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  // (void) env_var; // Silence unused variable warning
  // (void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
  // IMPLEMENT_ME();

  // use the setenv function to make env_var the name of it and val what's stored in it
  // 1 allows for it to be overwritten
  setenv(env_var, val, 1); 
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  // get the absolute path
  char* path = realpath(dir, NULL);

  // if there is no path
  if (path == NULL) {
    // print a fail message
    perror("ERROR: Failed to resolve path");
    return;
  }
  // change directory 
  if (chdir(path) == -1) {
    // it fails to change directories then print this error message
    perror("ERROR: Failed to change directory");
    // free the data
    free(path);
    return;
  }
  

  // Update the PWD variable with the new path
  setenv("PWD", path, 1);

  // Free the memory allocated by realpath
  free(path);
}



  // TODO: Change directory

  // TODO: Update the PWD environment variable to be the new current working
  // directory and optionally update OLD_PWD environment variable to be the old
  // working directory.
  //IMPLEMENT_ME();



// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // print error if the queue isnt made
  if (!job_queue_initialized) {
    fprintf(stderr, "ERROR: No background jobs running\n");
    return;
  }


  bool found = false;
  int length = length_JobQueue(&background_jobs);

  // want to find the job without destroying the queue order, 
  // so we can pop and push elements back
  for (int i = 0; i < length; i++) {
    Job job = peek_front_JobQueue(&background_jobs);
    pop_front_JobQueue(&background_jobs);

    if (job.job_id == job_id) {
      found = true;
      // send the signal to all processes in this job
      for (int p = 0; p < job.pid_count; p++) {
        if (kill(job.pids[p], signal) < 0) {
          perror("ERROR: Failed to send signal to process");
        }
      }
    }

    // push the job back into the queue to preserve it
    push_back_JobQueue(&background_jobs, job);
  }

  if (!found) {
    fprintf(stderr, "ERROR: No such job: [%d]\n", job_id);
  }
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory

  // create a cwd array
  char cwd[PATH_MAX];

  // get the working directory and store it in cwd array
  if (getwd(cwd) != NULL){
    // newline and print the cwd
    printf("%s\n", cwd);
  } else {
    // if it fails then print this error
    perror("ERROR: Failed to get current directory");
  }
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // if the job queue has never been initialized, there are no jobs to print
  if (!job_queue_initialized) {
        return;
    }

  // get the current number of background jobs in the queue
  int length = length_JobQueue(&background_jobs);

  // iterate through every job currently in the queue
  for (int i = 0; i < length; i++) {
    // peek at the job at the front of the queue
    Job job = peek_front_JobQueue(&background_jobs);

    // print the job's ID, the PID of its first process, and its command string
    print_job(job.job_id, job.pids[0], job.cmd);

    // rotate the queue: pop the front job and push it to the back 
    // so the queue order remains intact after inspection
    pop_front_JobQueue(&background_jobs);
    push_back_JobQueue(&background_jobs, job);
  }

  // Flush the buffer before returning
  fflush(stdout);
}



/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);
  // uses a switch to decide which commands are supposed to run 
  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);
  // uses a switch to decide which commands are supposed to run 
  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
pid_t create_process(CommandHolder holder) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  (void) p_in;  // Silence unused variable warning
  (void) p_out; // Silence unused variable warning
  (void) r_in;  // Silence unused variable warning
  (void) r_out; // Silence unused variable warning
  (void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process
  // IMPLEMENT_ME();

  CommandType type = get_command_type(holder.cmd);
  
  // if the command is a parent command make sure a parent runs its
  if (type == CD || type == EXPORT || type == KILL) {
    parent_run_command(holder.cmd);
    // no child being created 
    return -1;
  }
  // NOTE: /* SIGPIPE handles writes to pipes whose read end has been closed */
  // tracks the read end of the pipe from the previous process in a pipeline
  
  static int previous_pipe_read_fd = -1;
  int pipe_fds[2] = {-1, -1};
  // if this command needs to output to a pipe, create the pipe
  if (p_out) {
    if (pipe(pipe_fds) < 0) {
      perror("ERROR: Failed to create pipe");
      return -1;
    }
  }


  pid_t pid = fork();

  if(pid < 0){
    // fork fail
    perror("ERROR: Failed to fork");
    if (p_out) {
      close(pipe_fds[0]);
      close(pipe_fds[1]);
    }
    return -1;
  }
  else if(pid == 0){
    // --- RUN CHILD PROCESS ---

    // 1. handle ir OR pipe input
    if (r_in) {
      // open target file with appropriate permissions
      int fd_in = open(holder.redirect_in, O_RDONLY); 
      if (fd_in < 0) {
        perror("ERROR: Failed to open input file for redirection");
        exit(EXIT_FAILURE);
      }
      // replace standard input with new, opened file desc
      if (dup2(fd_in, STDIN_FILENO) < 0) {
        perror("ERROR: Failed to dup2 input file descriptor");
        close(fd_in);
        exit(EXIT_FAILURE);
      }
      close(fd_in);
    }
    else if (p_in) {
      // read input from the previous pipe's read end
      if (dup2(previous_pipe_read_fd, STDIN_FILENO) < 0) {
        perror("ERROR: Failed to dup2 pipe input");
        exit(EXIT_FAILURE);
      }
      close(previous_pipe_read_fd);
    }

  // output redirection (> or >>)
  if (r_out) {
    // choose flags based on whether append (>>) or truncate (>) is requested
    int flags = O_WRONLY | O_CREAT | (r_app ? O_APPEND : O_TRUNC);
    // 0644 is used in unix for output redirection in read/write mode for owner, read-only for others
    int fd_out = open(holder.redirect_out, flags, 0644);
    if (fd_out < 0) {
      perror("ERROR: Failed to open output file for redirection");
      exit(EXIT_FAILURE);
    }
    // functionality is similar to input redir, just for output
    if (dup2(fd_out, STDOUT_FILENO) < 0) {
      perror("ERROR: Failed to dup2 output file descriptor");
      /**************** REMEMBER TO CLOSE ORIGINAL FILE DESCRIPTORS TO PREVENT MEMORY LEAKS ****************/
      close(fd_out); 
      exit(EXIT_FAILURE);
    }
    close(fd_out);
  }
  else if (p_out) {
  // Write output to the current pipe's write end
    close(pipe_fds[0]); // child doesn't need to read from this pipe
    if (dup2(pipe_fds[1], STDOUT_FILENO) < 0) {
      perror("ERROR: Failed to dup2 pipe output");
      close(pipe_fds[1]);
      exit(EXIT_FAILURE);
    }
      close(pipe_fds[1]);
    }

    child_run_command(holder.cmd);
    exit(EXIT_SUCCESS);
  }
  else {
    // --- PARENT PROCESS ---

    // if there was a previous pipe read end open in the parent, close it
    if (previous_pipe_read_fd != -1) {
      close(previous_pipe_read_fd);
      previous_pipe_read_fd = -1;
    }

    // if created a pipe for this process, save the read end for the next process in line
    if (p_out) {
      close(pipe_fds[1]);
      previous_pipe_read_fd = pipe_fds[0];
    }

    // parent returns the child pid
    return pid;
  }
}



 // Run a list of commands
void run_script(CommandHolder* holders) {
  pid_t pids[100];
  int count = 0;

  if (holders == NULL)
    return;

  check_jobs_bg_status();

  // if the user tries to exit without any other commands, then exit
  if (get_command_holder_type(holders[0]) == EXIT && get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  // get what type of command it is
  CommandType type;

  // loop thru all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i) {
    // make a process and a pid
    pid_t pid = create_process(holders[i]);


    // check if a child pid was created 
    if (pid > 0) {
      // store the pid in the array and make sure we dont go over 100
      if (count < 100) {
        pids[count] = pid;
        count++;
      }
      else {
        // other print error message and wait for the child to finish
        fprintf(stderr, "ERROR: Too many child processes\n");
        waitpid(pid, NULL, 0);
      }
    }
  }
  // check if the job is not a background job
  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    // IMPLEMENT_ME();

    // wait for all child processes to finish
    for (int i = 0; i < count; i++) {
      waitpid(pids[i], NULL, 0);
    }
  }
  // handles background jobs, ensures that the there's at least one child process
  else if (count > 0) {
    // keeps track of next job id
    static int next_job_id = 1;
    // increments and assigns job_id to the next job
    int job_id = next_job_id++;

    // gets the original input command 
    char* cmd = get_command_string();

    // calls add job and stores the info, then prints it while it runs
    add_job(job_id, pids, count, cmd);
    print_job_bg_start(job_id, pids[0], cmd);

    // code for background job, not done yet.
    // TODO: Push the new job to the job queue
    // IMPLEMENT_ME();

    // TODO: Once jobs are implemented, uncomment and fill the following line
    
  }
}
