#ifndef __P2P_H__
#define __P2P_H__

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>


#define PEER_IP "127.0.0.1"
#define PORT_BASE 12000

#define BUF_LEN 2048
#define CMD_LEN 1024

// global variable
int ping_interval;


struct peer_info {
    char host[INET_ADDRSTRLEN];
    int port;
};

struct peer_node {
    
    struct peer_info first_succ;
    struct peer_info second_succ;
    struct peer_info self;
    struct peer_info first_predc;
    
    bool block;
    pthread_cond_t  t_lock;
    pthread_rwlock_t rw_lock;
    pthread_mutex_t mutex;
};

/* argument required for udp and tcp server threads */
struct args {
    struct peer_node * pnode_info;
    int fd;
};

/* args required for tcp client thread */
struct args_tcpclient {
    char* buf; // buf for data sending.
    int connect_to; // port to connect to
};

/* -----------------udp service-------------------- */


// Handlers for the udp threads.
int udp_resp_handler(void *info);
int udp_req_handler(void *info);
/* -----------------udp service-------------------- */


/* -----------------tcp service-------------------- */
enum {
    PEER_JOIN_CMD = 0, // I will join you peers 
    PEER_DEPARTURE_CMD, // I am killed 
    PEER_KILL1_CMD,   // the first successor are killed
    PEER_KILL2_CMD,    // the second successor are killed 
    DATA_INSERTION_CMD, // insert file
    DATA_RETRIEVAL_CMD, // retrieval file

    INFORM_PREDC_JOIN_CMD, // Inform predecessor the I joined.
    INFORM_JOIN_CMD, // Inform to get the successors.
    GET_FILE_CMD // File-holding port sends data to the one want to get
};


// Handlers for the tcp threads.
int tcp_req_establish (int connect_peer, char* buf);
int tcp_resp_handler(void *info);
int tcp_req_handler(int peer_id, int connect_peer, char* buf);


int task_processing(struct peer_node* pnode_info, int known_peer, 
        char cmd, int param);

void cmd_join(int fd, char* buf, struct peer_node* pnode_info);
void cmd_departure(int fd, char* buf, struct peer_node* pnode_info);
void cmd_kill1(int fd, char* buf, struct peer_node* pnode_info);
void cmd_kill2(int fd, char* buf, struct peer_node* pnode_info);
void cmd_insertion(int fd, char* buf, struct peer_node* pnode_info);
void cmd_retrieval(int fd, char* buf, struct peer_node* pnode_info);
void cmd_predc_join(int fd, char* buf, struct peer_node* pnode_info);
void cmd_inform_join(int fd, char* buf, struct peer_node* pnode_info);
void cmd_get_file(int fd, char* buf, struct peer_node* pnode_info);
/* -----------------tcp service-------------------- */

#endif