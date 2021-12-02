#include "p2p.h"
#include "utils.h"


/*

A good reference for C sockets programming (esp. structs and syscalls):
https://beej.us/guide/bgnet/html/multi/index.html

And for information on C Threads:
https://www.gnu.org/software/libc/manual/html_node/ISO-C-Threads.html

One of the main structs used in this program is the "sockaddr_in" struct.
See: https://beej.us/guide/bgnet/html/multi/ipstructsdata.html#structs

struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};

*/

int udp_server_init(struct peer_node* pnode_info, int peer_id);
int udp_client_init(struct peer_node* pnode_info, int peer_id);

int tcp_server_init(struct peer_node* pnode_info, int peer_id);
// int tcp_client_init(int connect_to, char* buf);

struct peer_node* peer_node_init(int peer_id);

struct args* new_args(struct peer_node* pnode_info, int fd);

struct args_tcpclient* new_args_tcpclient(int connect_to, char* buf);



int main(int argc, char *argv[]) {
    
    
    int peer_id;
    int first_succ, second_succ;
    struct peer_node* pnode_info;

    if ((argc != 6) && (argc != 5)) {
        printf("Parameter number invalid. \nargc: %d\n", argc);
        return -1;
    } else if ((strcmp(argv[1], "init"))&&(strcmp(argv[1], "join"))) {
        printf("Invalid for the first parameter. It should be 'init' or 'join'\n");
        printf("Your input: %s\n", argv[1]);
        return -1;
    }
    
    peer_id    = atoi(argv[2]);
    

    /* --------------------pnode_info initialization------------------------- */
    // pnode_info is used to store all the information required in a node.
    // Nearly all tasks need to use it.
    pnode_info = peer_node_init(peer_id);
    /* --------------------pnode_info initialization------------------------- */
    
    /* -----------------------------tcp server initialize--------------------------  */
    int tcp_resp_fd = tcp_server_init(pnode_info, peer_id);
    /* -----------------------------tcp server initialize--------------------------  */

    sleep(5);

    if ((!strcmp(argv[1], "init"))&&(argc==6)) {
        ping_interval = atoi(argv[5]);
        first_succ    = atoi(argv[3]);
        second_succ   = atoi(argv[4]);
        // get recorded infomation of successors
        pthread_mutex_lock(&pnode_info->mutex);
        pnode_info->first_succ.port  = first_succ;
        pnode_info->second_succ.port = second_succ;
        pthread_mutex_unlock(&pnode_info->mutex);
        
    } else if ((!strcmp(argv[1], "join"))&&(argc==5)) {
        char known_peer = atoi(argv[3]);
        ping_interval = atoi(argv[4]);
        printf("peer_id: %d\n", peer_id);
        printf("known_peer: %d\n", known_peer);
        

        sleep(5);
        task_processing(pnode_info, known_peer, PEER_JOIN_CMD, 0);
        
        pthread_mutex_lock(&pnode_info->mutex);
        pnode_info->block = 1;
        first_succ  = pnode_info->first_succ.port;
        second_succ = pnode_info->second_succ.port;
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
    }

    
    printf("peer_id           : %d\n", peer_id      );
    printf("first successor   : %d\n", first_succ   );
    printf("second successor  : %d\n", second_succ  );
    printf("ping interval     : %d\n", ping_interval);

    
    
    /* ------------------------udp service startup------------------------- */
    // initialize udp service threads
    int udp_resp_fd = udp_server_init(pnode_info, peer_id);
    int udp_req_fd = udp_client_init(pnode_info, peer_id);
    /* ------------------------udp service startup------------------------- */

    printf("[main] udp thread generation successfully\n");
    char input[CMD_LEN];
    char file_name[5];
    file_name[4] = '\0';
    int param;
    while (1) {
        
        scanf("%s", input);
        
        if (is_quit(input)) {
            task_processing(pnode_info, 0, PEER_DEPARTURE_CMD, 0);
            break;
        } else if (is_store(input)) {
            scanf("%d", &param);
            task_processing(pnode_info, 0, DATA_INSERTION_CMD, param);
        } else if (is_request(input)) {
            // cmd = DATA_RETRIEVAL_CMD;
            scanf("%d", &param);
            task_processing(pnode_info, 0, DATA_RETRIEVAL_CMD, param);
        } else {
            printf("%s\n", input);
        }
        
        // Equivalent to `sleep(0.1)`
        usleep(100000);
    }


    // This code will never be reached, but assuming there was some way
    // to tell the server to shutdown, this code should happen at that
    // point.

    // Close the sockets
    close(udp_resp_fd);
    close(udp_req_fd);
    close(tcp_resp_fd);

    
    
    pthread_rwlock_destroy(&pnode_info->rw_lock);
    pthread_mutex_destroy(&pnode_info->mutex);
    pthread_cond_destroy(&pnode_info->t_lock);


    free(pnode_info);

    printf("exit!\n");
    return 0;

    
}
/* ------------------task processing---------------------- */

int task_processing(struct peer_node* pnode_info, int known_peer, char cmd, int param) {

    char buf[BUF_LEN];
    int connect_to;

    // acquire read lock
    pthread_mutex_lock(&pnode_info->mutex);
    int peer_id    = pnode_info->self.port;
    int first_succ = pnode_info->first_succ.port;
    int first_predc = pnode_info->first_predc.port;
    int second_succ = pnode_info->second_succ.port;
    // printf("---------------%d\n", param);

    *buf = (char) cmd;
    if (cmd == PEER_JOIN_CMD) { // join cmd
        
        // param format
        *(buf+1) = (char) peer_id; // node id prepare to insert
        
        connect_to = known_peer;// connect to a known peer to join the network
    
    } else if (cmd == PEER_DEPARTURE_CMD) { // departure cmd
        pnode_info->block  = 1; // to stall udp service

        // param format
        *(buf+1) = (char) peer_id;    // id to quit
        *(buf+2) = (char) first_succ; // first successor
        *(buf+3) = (char) second_succ; // second successor
        
        connect_to = first_predc; // connect to the first predecessor, informing it my successor
    
    } else if (cmd == PEER_KILL1_CMD) {
        
        // param format
        *(buf+1) = (char) peer_id; // id to acquire second successor (and first is the previous second)
        
        connect_to = second_succ;// connect to the first successor, fetching successor
    
    } else if (cmd == PEER_KILL2_CMD) {

        // param format
        *(buf+1) = (char) peer_id; // id to acquire second successor

        connect_to = first_succ;// connect to the first successor

    } else if (cmd == DATA_INSERTION_CMD) {// Store cmd
        int store_port = param & 0xff;
        char file_str[6];
        int2str4(param, file_str);

        if (((first_predc < store_port)&&(store_port < peer_id)) ||
                ((first_predc < store_port)&&(first_predc > peer_id)) ||
                ((store_port < peer_id)&&(first_predc > peer_id)) ||
                (store_port == peer_id)) {// if file is need to store locally
            printf("Store %s request accepted\n", file_str);
            
            pthread_mutex_unlock(&pnode_info->mutex); // release write lock
            return 0;
        }

        printf("Store %s request forwarded to my successor\n", file_str);
        
        // param format
        *(buf+1) = (char) 0;  // reserved
        *(buf+2) = (char) 0;  // reserved
        *(buf+3) = (char) 0;  // reserved
        *(buf+4) = (char) param&0xff; // file name to store
        *(buf+5) = (char) (param>>8)&0xff; // file name to store
        *(buf+6) = (char) (param>>16)&0xff; // file name to store
        *(buf+7) = (char) (param>>24)&0xff; // file name to store

        // printf("%d-------------------\n", param);
        connect_to = first_succ;// connect to the first successor

    } else if (cmd == DATA_RETRIEVAL_CMD) {  // Request cmd
        int store_port = param & 0xff;
        char file_str[6];
        int2str4(param, file_str);
        if (((first_predc < store_port)&&(store_port < peer_id)) ||
                ((first_predc < store_port)&&(first_predc > peer_id)) ||
                ((store_port < peer_id)&&(first_predc > peer_id)) ||
                (store_port == peer_id)) {// if the file is local.

            printf("File %s is stored here\n", file_str);
            printf("Sending file %s to Peer %d\n", file_str, peer_id);
            printf("The file has been sent\n");
            printf("Peer %d had File %s\n", peer_id, file_str);
            printf("Receiving File %s from Peer %d\n", file_str, peer_id);
            printf("File %s received\n", file_str);

            pthread_mutex_unlock(&pnode_info->mutex); // release write lock
            return 0;
        }

        printf("File request for %s has been sent to my successor\n", file_str);
        
        // param format
        *(buf+1) = (char) peer_id;  // who need the file
        *(buf+2) = (char) 0;        // reserved
        *(buf+3) = (char) 0;        // reserved
        *(buf+4) = (char) param&0xff; // file name to get
        *(buf+5) = (char) (param>>8)&0xff; // file name to get
        *(buf+6) = (char) (param>>16)&0xff; // file name to get
        *(buf+7) = (char) (param>>24)&0xff; // file name to get
        
        connect_to = first_succ; // connect to the first successor
    }
    pthread_mutex_unlock(&pnode_info->mutex); // release write lock
    
    // tcp_client_init(connect_to, buf);
    tcp_req_handler(peer_id, connect_to, (char*)buf);
    return 0;
}

/* ------------------joining peer---------------------- */


/* ------------------initialization function------------------ */

struct peer_node* peer_node_init (int peer_id) {
    struct peer_node* pnode_info_ptr = calloc(1, sizeof(struct peer_node));
    strcpy(pnode_info_ptr->first_succ.host,  PEER_IP);
    strcpy(pnode_info_ptr->second_succ.host, PEER_IP);
    strcpy(pnode_info_ptr->self.host, PEER_IP);
    strcpy(pnode_info_ptr->first_predc.host, PEER_IP);
    pnode_info_ptr->self.port        = peer_id;

    // It would be a good idea to check the return value of these inits,
    // to make sure that they succeeded.
    pthread_cond_init(&pnode_info_ptr->t_lock, NULL);
    pthread_mutex_init(&pnode_info_ptr->mutex, NULL);
    pthread_rwlock_init(&pnode_info_ptr->rw_lock, NULL);

    pnode_info_ptr->block  = 0;

    return pnode_info_ptr;
}

struct args* new_args(struct peer_node* pnode_info, int fd) {
    struct args* args = calloc(1, sizeof(struct args));
    args->pnode_info = pnode_info;
    args->fd = fd;
    return args;
}

struct args_tcpclient* new_args_tcpclient(int connect_to, char* buf) {
    struct args_tcpclient* args = calloc(1, sizeof(struct args_tcpclient));
    args->buf        = buf;
    args->connect_to = connect_to;
    return args;
}

// int tcp_client_init(int connect_to, char* buf) {
    
//     struct args_tcpclient* tcpclient_args = new_args_tcpclient(connect_to, buf);

//     // Create the threads.
//     pthread_t tcpreq_thread;

//     pthread_create(&tcpreq_thread, NULL, (void *(*)(void *))tcp_req_handler, (void *) tcpclient_args);

//     return 0;
// }


int tcp_server_init(struct peer_node* pnode_info, int peer_id) {
    
    int tcp_resp_fd = socket(AF_INET, SOCK_STREAM, 0);

    int flags = fcntl(tcp_resp_fd, F_GETFL, 0);
    fcntl(tcp_resp_fd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in sockaddr_local = {0};
    fill_sockaddr(&sockaddr_local, PEER_IP, PORT_BASE+peer_id);

    printf("TCP Binding...\n");
    bind(tcp_resp_fd, (struct sockaddr *) &sockaddr_local, sizeof(sockaddr_local));

    
    struct args* tcpserver_args = new_args(pnode_info, tcp_resp_fd);

    // Create the threads.
    pthread_t tcpresp_thread;

    pthread_create(&tcpresp_thread, NULL, (void *(*)(void *))tcp_resp_handler, (void *) tcpserver_args);
    
    return tcp_resp_fd;
}

int udp_client_init(struct peer_node* pnode_info, int peer_id) {
    
    // udp socket initialization
    int udp_req_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Create an args struct for each thread. Both structs have a
    // pointer to the same pnode, but different sockets (different file
    // descriptors).
    struct args* udpclient_args = new_args(pnode_info, udp_req_fd);

    // Create the threads.
    pthread_t udpreq_thread;

    pthread_create(&udpreq_thread,  NULL, (void *(*)(void *))udp_req_handler, (void *) udpclient_args);

    return udp_req_fd;
}

int udp_server_init(struct peer_node* pnode_info, int peer_id) {

    // Create the server's socket.
    //
    // The first parameter indicates the address family; in particular,
    // `AF_INET` indicates that the underlying network is using IPv4.
    //
    // The second parameter indicates that the socket is of type
    // SOCK_DGRAM, which means it is a UDP socket (rather than a TCP
    // socket, where we use SOCK_STREAM).
    //
    // This returns a file descriptor, which we'll use with our sendto /
    // recvfrom functions later.
    int udp_resp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Create the sockaddr that the server will use to send data to the
    // client.
    struct sockaddr_in sockaddr_to;
    fill_sockaddr(&sockaddr_to, PEER_IP, PORT_BASE+peer_id);

    // Let the server reuse the port if it was recently closed and is
    // now in TIME_WAIT mode.
    const int so_reuseaddr = 1;
    setsockopt(udp_resp_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(int));

    printf("UDP Server Binding...\n");
    bind(udp_resp_fd, (struct sockaddr *) &sockaddr_to, sizeof(sockaddr_to));    
    
    // Create an args struct for each thread. Both structs have a
    // pointer to the same pnode, but different sockets (different file
    // descriptors).
    struct args* udpserver_args = new_args(pnode_info, udp_resp_fd);

    // Create the threads.
    pthread_t udpresp_thread;

    pthread_create(&udpresp_thread, NULL, (void *(*)(void *))udp_resp_handler, (void *) udpserver_args);
    

    return udp_resp_fd;
}

/* ------------------initialization function------------------ */






