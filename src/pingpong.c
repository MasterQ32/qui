#include <stdio.h>
#include <stdlib.h>

#include <syscall.h>

int main(int argc, char ** argv)
{
	printf("Type text to get an echo:\n");
	while(true)
	{
		char buffer[1];
		int len = fread(buffer, 1, 1, stdin);
		if(len <= 0) {
			return EXIT_SUCCESS;
		}
		fwrite(buffer, 1, 1, stdout);

		yield();
	}
}
