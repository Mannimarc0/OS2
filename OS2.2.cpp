#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <fcntl.h>      
#include <unistd.h>     
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <sys/time.h>   
#include <sys/select.h> 

int fd;
void *ptr;
#define FILENAME "shared_mem.bin"
#define FILESIZE 4096

void print_menu_server() {
    printf("1. Perform projection\n");
    printf("2. Write Data\n");
    printf("3. End Work\n");
    printf("0. Exit\n");
}

void print_menu_client() {
    printf("1. Perform projection\n");
    printf("2. Read Data\n");
    printf("3. End Work\n");
    printf("0. Exit\n");
}

int main() {
    int ch;
    char* choice = (char*)malloc(100);
    
    printf("Enter mode (Client/Server): ");
    if (scanf("%99s", choice) != 1) {
        perror("Error reading choice");
        exit(EXIT_FAILURE);
    }

    if (strcmp(choice, "Client") == 0) {
        printf("Client mode\n");
        do {
            print_menu_client();
            if (scanf("%d", &ch) != 1) break;
            switch (ch) {
                case 1: 
                    fd = open(FILENAME, O_RDONLY);
                    ptr = mmap(NULL, FILESIZE, PROT_READ, MAP_SHARED, fd, 0);
                    if (ptr == MAP_FAILED) {
                        perror("Error mapping file");
                        close(fd);
                        break;
                    }
                    break;
                case 2: {
                    fd_set read_fds;
                    struct timeval timeout = {5, 0};
                    FD_ZERO(&read_fds);
                    FD_SET(fd, &read_fds);
                    int result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
                    
                    if (result == -1) {
                        perror("Error in select");
                        munmap(ptr, FILESIZE);
                        close(fd);
                        exit(EXIT_FAILURE);
                    } else if (result == 0) {
                        printf("Timeout occurred, no data to read\n");
                    } else {
                        if (FD_ISSET(fd, &read_fds)) {
                            printf("Message from server: %s\n", (char*)ptr);
                        }
                    }
                    break;
                }
                case 3:
                    munmap(ptr, FILESIZE);
                    close(fd);
                    break;
                case 0: break;
                default: printf("Invalid choice\n"); break;
            }
        } while (ch != 0);

    } else if (strcmp(choice, "Server") == 0) {
        printf("Server mode\n");
        do {
            print_menu_server();
            if (scanf("%d", &ch) != 1) break;
            switch (ch) {
                case 1:
                    fd = open(FILENAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    if (fd == -1) {
                        perror("Error opening file");
                        exit(EXIT_FAILURE);
                    }
                    if (ftruncate(fd, FILESIZE) == -1) {
                        perror("Error setting file size");
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                    ptr = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                    if (ptr == MAP_FAILED) {
                        perror("Error mapping file");
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 2:
                    sprintf((char*)ptr, "Hello, shared memory!");
                    printf("Data written to memory.\n");
                    break;
                case 3:
                    munmap(ptr, FILESIZE);
                    unlink(FILENAME);
                    close(fd);
                    break;
                case 0: break;
                default: printf("Invalid choice\n"); break;
            }
        } while (ch != 0);
    } else {
        printf("Invalid mode\n");
    }

    free(choice);
    return 0;
}
