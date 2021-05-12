
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int _, char* const* argv)
{
	int error = 0, wstatus;
	argv++;
	
	struct timeval before, after, diff;
	
	gettimeofday(&before, NULL);
	
	pid_t child;
	if (child = fork(), child < 0)
		perror("fork"),
		error = 1;
	else if (!child && execvp(*argv, argv) < 0)
		perror("execvp"),
		error = 1;
	else if (waitpid(child, &wstatus, 0) < 0)
		perror("waitpid"),
		error = 1;
	
	gettimeofday(&after, NULL);
	
	timersub(&after, &before, &diff);
	
	if (!error)
		printf("time: %.1f\n", diff.tv_sec + diff.tv_usec / 1000000.0);
	
	return error ?: wstatus;
}


