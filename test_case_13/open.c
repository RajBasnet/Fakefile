
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int _, const char** argv)
{
	int error = 0, fd;
	
	if (fd = open(argv[1], O_CREAT | O_TRUNC | O_RDWR, 0664), fd < 0)
		perror("open"),
		error = 1;
	
	printf("fd == %i\n", fd);
	
	return error;
}

