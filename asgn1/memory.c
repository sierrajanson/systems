#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

int fatal_error(const char *message){
	fprintf(stderr,"%s", message);
	exit(1);
}


int main(int argc, char **argv) {
	char user_c;
	char user_input[4];
	int user_count = 0;
	int user_success = 0;
	
	while(user_count < 4 && read(STDIN_FILENO, &user_c, 1) > 0){
		if (user_c == '\n'){
			user_success = 1;
			break;
			// check if there's stuff after
		}
		user_input[user_count] = user_c;	
		user_count++;
	}
	if (user_success != 1) fatal_error("Invalid Command\n");

    if (strncmp(user_input, "set",3) == 0) { // set --> set\n<location>\n<content_length>\n<contents>
	// initialize FILENAME
	char filename[PATH_MAX]; // dynamically allocate
        int res = scanf("%s", filename);
    	
	// check for valid read
	if (res <= 0) fatal_error("Invalid Command\n");
	
	// check for valid text file
	char *txt = strstr(filename, ".txt");
	if (txt == NULL) fatal_error("Invalid Command\n");

	// create or open file
	// does this check for directories?? !!
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP);
        if (fd == -1) fatal_error("Invalid Command\n");

	// initialize FILELENGTH
	int length = 0;
        res = scanf("%d", &length);
    	if (res <= 0) fatal_error("Invalid Command\n");
	if (length == 0) {
		write(STDOUT_FILENO, "OK\n",3);
        	close(fd);
		exit(0);
	}
	// CHANGE TO 10 MB
        char buffer[PATH_MAX];
	char c;
	int count = 0;
	while(count < PATH_MAX && read(STDIN_FILENO, &c, 1) > 0){
		if (c == '\n'){
			user_success = 1;
			break;
			// check if there's stuff after
		}
		buffer[count] = c;	
		count++;
	}
	// append null terminator??
	
	int msg_len = strlen(buffer);
	// partial write	
	if (length < msg_len){
		// may need to append null terminator
	} else if (length > msg_len){ // not enough of msg to write
		length = msg_len; // shorten length
	} else { // ideal case where they are equal
		
	}
	int bytes = 0;
	
        while (bytes < length) {
            bytes += write(fd, buffer, length - bytes);
        }
    	
	// successful finish
	write(STDOUT_FILENO, "OK\n",3);
        close(fd);
	//check that file has closed !!

    } else if (strncmp(user_input, "get",3) == 0) { // get
        //int res = scanf(" %[^\n]", filename);
        // edge case: what if 4096 passed and no newline --> invalid?
	// null characters necessary
	/*
	read in one character at a time. if still reading by 4096 and no newline has been reached, exit with fatal error
	*/
	// READING IN FILENAME
	// checks --> if filename is too long
	//        --> if filename is not terminated by a newline character
	//        --> if filename is termiated by \n but there is still following
	char filename[PATH_MAX];
	int count = 0;
	int success = 0;
	char c;
	while(count < PATH_MAX && read(STDIN_FILENO, &c, 1) > 0){
		if (c == '\n'){
			success = 1;
			break;
			// check if there's stuff after
		}
		filename[count] = c;
		count+=1;
	}
	// no \n, or filename too long
	if (success != 1) fatal_error("Invalid Command\n");
	
	int fd = open(filename, O_RDONLY, 0);	
	
	// may not exceed 10,000,000 bytes
        if (fd == -1) fatal_error("Invalid Command\n");
        char buffer[4096];

        ssize_t bytes = 0;
        while ((bytes = read(fd, buffer, 16)) > 0) {
            for (int i = 0; i < bytes; i++) {
                fprintf(stdout, "%c", buffer[i]);
            }
        }

        close(fd);

    } else { // error
       fatal_error("Invalid Command\n");
    }

    (void) argc;
    (void) argv;
    return 0;
}
