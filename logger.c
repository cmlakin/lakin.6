#include "oss.h"
#include "logger.h"

void logger(const char * string_buf) {
    int fid;
    fid = open(LOG_FILENAME, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (fid == -1) {
        snprintf(perror_buf, sizeof(perror_buf), "%s: open: ", perror_arg0);
        perror(perror_buf);
    }

    write(fid, (void *) string_buf, strlen(string_buf));
    close(fid);
}
