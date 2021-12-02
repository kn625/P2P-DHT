#ifndef __UTILS_H__
#define __UTILS_H__

#include "p2p.h"

////////////////////////////////////////////////////////////////////////
// Socket helper functions

// Wrapper for the recv function, to avoid code duplication.
int recv_wrapper(int fd, char *buf, struct sockaddr_in *sockaddr_from,
        socklen_t *from_len, char *who);

// Wrapper for the sendto function, to avoid code duplication.
int send_wrapper(int fd, char *buf, struct sockaddr_in *sockaddr_to,
        socklen_t *to_len, char *who);

// Get a peer_info struct based on the sockaddr we're communicating with.
struct peer_info get_peer_info(struct sockaddr_in *sa);

// Get the "name" (IP address) of who we're communicating with.
// char *get_name(struct sockaddr_in *sa, char *name);

// Populate a sockaddr_in struct with the specified IP / port to connect to.
void fill_sockaddr(struct sockaddr_in *sa, char *ip, int port);

// "Print" out a client, by copying its host/port into the specified buffer.
char *print_peer_buf(struct peer_info peer, char *buf);

void print_peer(struct peer_info peer);

// Compare two clients to see whether they have the same host/port.
int peers_equal(struct peer_info a, struct peer_info b);

// A wrapper function for fgets, similar to Python's built-in 'input' function.
void get_input(char *buf, char *msg);

// Get a string containing the current date/time.
char *get_time(void);

// int to string of length 4
int int2str4(int num, char* num4_str);


int is_quit(char* input);
int is_store(char* input);
int is_request(char* input);
////////////////////////////////////////////////////////////////////////


#endif