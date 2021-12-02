

#include "p2p.h"
#include "utils.h"

int udp_resp_handler(void *args_) {
    struct args* args = (struct args*) args_;
    struct peer_node* pnode_info = args->pnode_info;
    int udp_respond_fd = args->fd;

    // Array that we'll use to store the data we're sending / receiving.
    char buf[BUF_LEN + 1] = {0};
    


    // Create the sockaddr that the server will use to receive data from
    // the client (and to then send data back).
    struct sockaddr_in sockaddr_from;

    // We need to create a variable to store the length of the sockaddr
    // we're using here, which the recvfrom function can update if it
    // stores a different amount of information in the sockaddr.
    socklen_t from_len = sizeof(sockaddr_from);

    while (1) {

        // The `recv_wrapper` function wraps the call to recv, to avoid
        // code duplication.
        recv_wrapper(udp_respond_fd, buf, &sockaddr_from, &from_len, "respond");
        // printf("[respond] buf: %s\n", buf);
        if (buf[0] == '1') { // stands for the first predecessor
            printf("[respond] Ping request message received from Peer %d\n", atoi(buf+1));
            
            // acquire lock for structure pnode_info
            pthread_mutex_lock(&pnode_info->mutex);
            pnode_info->first_predc.port = atoi(buf+1);
            // release lock for structure pnode_info
            pthread_mutex_unlock(&pnode_info->mutex);

            // // Get the current time.
            // char *curr_time = get_time();
            // printf("[respond] Current time is %s", curr_time);

            printf("[respond] first predecessor is %d\n", atoi(buf+1));

        } else if (buf[0] == '2') {// stands for the second predecessor
            printf("[respond] Ping request message received from Peer %d\n", atoi(buf+1));
        }


        snprintf(buf, BUF_LEN, "ping info received.");
        send_wrapper(udp_respond_fd, buf, &sockaddr_from, &from_len, "respond");
    }

    return EXIT_SUCCESS;
}

int udp_req_handler(void *args_) {
    struct args* args = (struct args*) args_;
    struct peer_node* pnode_info = args->pnode_info;
    int udp_request_fd = args->fd;
    


    // Array that we'll use to store the data we're sending / receiving.
    char buf[BUF_LEN + 1] = {0};

    // Temporary array used to store the name of the client when printing.
    char name[BUF_LEN] = {0};
    // int init = 1;

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(udp_request_fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        printf("Error\n"); 
    } 


    bool block;
    char last_disconn = 0;
    bool first_conn = 0;
    bool second_conn = 0;

    sleep(2);

    while (1) {
        
        do {
            pthread_mutex_lock(&pnode_info->mutex);
            block = pnode_info->block;
            pthread_mutex_unlock(&pnode_info->mutex);
        } while (block);

        // acquire read lock for structure pnode_info
        pthread_mutex_lock(&pnode_info->mutex);
        

        printf("[request] Ping requests sent to Peers %d and %d\n", 
                pnode_info->first_succ.port, pnode_info->second_succ.port);
        
        struct sockaddr_in sockaddr_to = {0};
        socklen_t to_len = sizeof(sockaddr_to);
        

        /* Send request to first successor */
        snprintf(buf, BUF_LEN, "%d%d", 1, pnode_info->self.port);
        fill_sockaddr(&sockaddr_to, pnode_info->first_succ.host, PORT_BASE+pnode_info->first_succ.port);
        send_wrapper(udp_request_fd, buf, &sockaddr_to, &to_len, "request");

        /* Send request to second successor */
        snprintf(buf, BUF_LEN, "%d%d", 2, pnode_info->self.port);
        fill_sockaddr(&sockaddr_to, pnode_info->second_succ.host, PORT_BASE+pnode_info->second_succ.port);
        send_wrapper(udp_request_fd, buf, &sockaddr_to, &to_len, "request");


        struct sockaddr_in sockaddr_from = {0};
        socklen_t from_len = sizeof(sockaddr_from);
        struct peer_info addr_resp;

        int ret = recv_wrapper(udp_request_fd, buf, &sockaddr_from, &from_len, "request");

        addr_resp = get_peer_info(&sockaddr_from);
        if (peers_equal(addr_resp, pnode_info->first_succ)) {
            first_conn = 1;
            printf("[request] Ping response received from Peer %d\n", pnode_info->first_succ.port);
        } else if (peers_equal(addr_resp, pnode_info->second_succ)) {
            second_conn = 1;
            printf("[request] Ping response received from Peer %d\n", pnode_info->second_succ.port);
        }
        // printf("[request] first conn %d\n", first_conn);
        // printf("[request] second conn %d\n", second_conn);

        ret = recv_wrapper(udp_request_fd, buf, &sockaddr_from, &from_len, "request");
        if (ret <= 0) {
            last_disconn += 1;
        } else {
            last_disconn = 0;
        }
        addr_resp = get_peer_info(&sockaddr_from);
        if (peers_equal(addr_resp, pnode_info->first_succ)) {
            first_conn = 1;
            printf("[request] Ping response received from Peer %d\n", pnode_info->first_succ.port);
        } else if (peers_equal(addr_resp, pnode_info->second_succ)) {
            second_conn = 1;
            printf("[request] Ping response received from Peer %d\n", pnode_info->second_succ.port);
        }
        // printf("[request   ] first conn %d\n", first_conn);
        // printf("[request   ] second conn %d\n", second_conn);
        // release lock for structure pnode_info
        pthread_mutex_unlock(&pnode_info->mutex);

        
        if ((last_disconn==2) && (first_conn == 0)) {
            // printf("[request] disconnect first successor-------------\n");
            task_processing(pnode_info, pnode_info->second_succ.port, 
                    PEER_KILL1_CMD, 0);
            sleep(10);
        } else if ((last_disconn==2) && (second_conn == 0)) {
            // sleep(5);
            // printf("[request] disconnect second successor-------------\n");
            task_processing(pnode_info, pnode_info->first_succ.port, 
                    PEER_KILL2_CMD, 0);
            sleep(10);
        }
        first_conn = 0;
        second_conn = 0;
        sleep(ping_interval);
        
    }
    return EXIT_SUCCESS;
}