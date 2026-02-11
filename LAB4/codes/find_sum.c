#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"


int main(int argc, char *argv[]) {
    int total_sum = 0;


    for (int i = 1; i < argc; i++) {
        char *p = argv[i];

     \
        while (*p) {

            if (*p < '0' || *p > '9') {
                p++;
                continue;
            }

            
            int current_num = 0;
            while (*p >= '0' && *p <= '9') {
                current_num = current_num * 10 + (*p - '0');
                p++;
            }
            total_sum += current_num;
        }
    }

   
    int fd;
    fd = open("result.txt", O_CREATE | O_WRONLY);

    if (fd < 0) {
        printf(2, "Error: cannot open result.txt for writing\n");
        exit();
    }

   
    printf(fd, "%d\n", total_sum);
    close(fd);

    exit();
}