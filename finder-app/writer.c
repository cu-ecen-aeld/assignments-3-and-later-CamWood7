#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
	
	openlog("assignment2Logs", LOG_PID, LOG_USER);

	// End call if not enough inputs
	if (argc != 3) {
	
		fprintf(stderr, "Not enough inputs");
		exit(EXIT_FAILURE);
	
	}

	// Set up inputs
	char *string = argv[2];
	char *file_name = argv[1];

	FILE *file = fopen(file_name, "w");

	// Error if no file
	if (file == NULL) {
		syslog(LOG_ERR, "File %s does not exist", file_name);
		perror("Error");
		exit(EXIT_FAILURE);
	}

	// Attempt to write to file
	if (fprintf(file,"%s\n", string) < 0) {
	
		syslog(LOG_ERR, "Failed to write to file %s", file_name);
		perror("Error");
		fclose(file);
		exit(EXIT_FAILURE);
	
	}	

	// Close File
	fclose(file);

	// Log Success
	syslog(LOG_DEBUG, "Writing %s to %s", string, file_name);

	closelog();
	return EXIT_SUCCESS;

}
