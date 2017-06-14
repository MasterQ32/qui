#include <stdio.h>
#include <stdlib.h>

#include <init.h>
#include <rpc.h>

int main(int argc, char ** argv)
{
	stdout = NULL; // Console only!
	
	printf("Hello, Client!\n");
	pid_t svc;
	while((svc = init_service_get("gui")) == 0) {
		// ?
	}
	
	uint32_t result = rpc_get_dword(svc, "WNCREATE", 0, NULL);
	
	printf("Got result: %u\n", result);
	
	return EXIT_SUCCESS;
}