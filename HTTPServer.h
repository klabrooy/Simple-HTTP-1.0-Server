#ifndef HTTPSERVER_H_SEEN
#define HTTPSERVER_H_SEEN

/* Thread process */
void *process(void *ts);
/* Handles sigpipe (thread tries to write to a socket that has been shutdown) */
void sigpipe_handler(int signo);
/* Concatenates three strings a, b, c using memcpy */
char *concatenate(const char *a, const char *b, const char *c);


#endif /* !HTTPSERVER_H_SEEN */
