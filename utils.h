#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>

/*
 * @brief Parses an integer from a string
 * @param dest pointer where the integer must be stored
 * @param str source string
 * @param base base of the number (10 if decimal)
 * @return 0 on success, otherwise means errors occurred
 */
int parse_int(int* dest, char* str, int base);

/*
 * @brief Parses a long integer from a string
 * @param dest pointer where the long integer must be stored
 * @param str source string
 * @param base base of the number (10 if decimal)
 * @return 0 on success, otherwise means errors occurred
 */
int parse_long(long* dest, char* str, int base);

/*
 * @brief Returns the filename from it's complete path. For example, if path is "/tmp/fb_1234" return is "fb_1234"
 * @param path original file pathname
 * @return filename
 */
char *filenameFromPath(const char* path);

/*
 * @brief Attempts to lock the specified mutex with a blocking call
 * @param mutex pointer to the mutex
 * @param name name of the "owner" of the mutex for debugging
 * @param debug if true (!= 0), debuggin is made active (prints are made to see the progress of the lock)
 * @return 0 on success, otherwise means errors occurred
 */
int attempt_mutex_lock(pthread_mutex_t *mutex, char *name, int debug);

/*
 * @brief Attempts to unlock the specified mutex with a blocking call
 * @param mutex pointer to the mutex
 * @param name name of the "owner" of the mutex for debugging
 * @param debug if true (!= 0), debuggin is made active (prints are made to see the progress of the unlock)
 * @return 0 on success, otherwise means errors occurred
 */
int attempt_mutex_unlock(pthread_mutex_t *mutex, char *name, int debug);

/*
 * @brief Attempts to destroy the specified mutex with a blocking call
 * @param mutex pointer to the mutex
 * @param name name of the "owner" of the mutex for debugging
 * @param debug if true (!= 0), debuggin is made active (prints are made to see the progress of the destruction)
 * @return 0 on success, otherwise means errors occurred
 */
int attempt_mutex_destroy(pthread_mutex_t *mutex, char *name, int debug);

#endif
