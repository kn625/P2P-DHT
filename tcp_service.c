#include "p2p.h"
#include "utils.h"




int tcp_req_handler(int peer_id, int connect_peer, char* buf) {

    /* establishing send tcp connection */
    int tcp_req_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sockaddr_local = {0};
    fill_sockaddr(&sockaddr_local, PEER_IP, PORT_BASE+peer_id+1000);

    printf("[req] TCP Binding...\n");
    bind(tcp_req_fd, (struct sockaddr *) &sockaddr_local, sizeof(sockaddr_local));

    struct sockaddr_in sockaddr_to = {0};
    socklen_t to_len = sizeof(sockaddr_to);
    fill_sockaddr(&sockaddr_to, PEER_IP, PORT_BASE+connect_peer);

    
    // printf("\n\n\n[req] request to peer\n");
    // struct peer_info tmp = get_peer_info(&sockaddr_to);
    // print_peer(tmp);
    
    // printf("[req] TCP req connected\n");
    // printf("[req] buf[0] %d\n", buf[0]);
    // printf("[req] buf[1] %d\n", buf[1]);
    // printf("[req] buf[2] %d\n", buf[2]);
    // printf("[req] buf[3] %d\n", buf[3]);
    // printf("[req] buf[4] %d\n", buf[4]);
    // printf("[req] buf[5] %d\n", buf[5]);
    // printf("[req] buf[6] %d\n", buf[6]);
    // printf("[req] buf[7] %d\n", buf[7]);
    // printf("[req] buf[8] %d\n", buf[8]);

    while (connect(tcp_req_fd, (struct sockaddr *) &sockaddr_to, to_len)) {
        printf("[req] TCP req connect faild, try again...\n");
        sleep(1);
    }
    
    

    int ret = send(tcp_req_fd, buf, BUF_LEN, 0);


    printf("[req] Data sent successfully %d\n", ret);

    close(tcp_req_fd);
    return 0;
}


int tcp_req_establish(int connect_peer, char* buf) {

    /* establishing send tcp connection */
    int tcp_req_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sockaddr_to = {0};
    socklen_t to_len = sizeof(sockaddr_to);
    fill_sockaddr(&sockaddr_to, PEER_IP, PORT_BASE+connect_peer);


    printf("\n\n\n[est] request to peer\n");
    struct peer_info tmp = get_peer_info(&sockaddr_to);
    print_peer(tmp);
    

    if (connect(tcp_req_fd, (struct sockaddr *) &sockaddr_to, to_len)) {
        printf("[est] TCP req connect faild, try again...\n");
        sleep(1);
        if (connect(tcp_req_fd, (struct sockaddr *) &sockaddr_to, to_len)) {
            printf("[est] TCP est connect faild. Return\n");
            return -1;
        }
    }
    
    printf("[est] TCP req connected\n");

    return tcp_req_fd;
}

/* -----------------------Respond------------------------ */
int tcp_resp_handler(void* args_) {
    struct args* args = (struct args*) args_;
    struct peer_node* pnode_info = args->pnode_info;
    int tcp_resp_fd = args->fd;

    char cmd;
    char buf[BUF_LEN];

    printf("[resp] entering tcp successfully!\n");


    while (1) {
        
        /* establishing receive tcp connection */
        // printf("[resp] TCP listening...\n");
        int ret = listen(tcp_resp_fd, 10);
        if (ret != 0) {
            printf("listen error, try again...\n");
            continue;
        }
        // printf("[resp] TCP listened\n");
        struct sockaddr_in sockaddr_from = {0};
        socklen_t from_len = sizeof(sockaddr_from);
        int tcp_resp_join_fd = accept(tcp_resp_fd, 
                (struct sockaddr *) &sockaddr_from, &from_len);
        
        if (tcp_resp_join_fd <= 0) {
            // printf("accept error, restart listening...\n");
            usleep(100000);
            continue;
        }
        
        printf("\n\n\n[resp] accept from peer:\n");
        struct peer_info tmp = get_peer_info(&sockaddr_from);
        print_peer(tmp);

        int num = recv(tcp_resp_join_fd, buf, BUF_LEN, 0);
        while (num <= 0) {
            num = recv(tcp_resp_join_fd, buf, BUF_LEN, 0);
        }

        cmd = buf[0];
        // printf("[resp] buf[0] %d\n", buf[0]);
        // printf("[resp] buf[1] %d\n", buf[1]);
        // printf("[resp] buf[2] %d\n", buf[2]);
        // printf("[resp] buf[3] %d\n", buf[3]);
        // printf("[resp] buf[4] %d\n", buf[4]);
        // printf("[resp] buf[5] %d\n", buf[5]);
        // printf("[resp] buf[6] %d\n", buf[6]);
        // printf("[resp] buf[7] %d\n", buf[7]);
        // printf("[resp] buf[8] %d\n", buf[8]);
        // printf("[resp] data received successfully!%d\n", num);

        switch(cmd) {
            case PEER_JOIN_CMD:
                cmd_join(tcp_resp_fd, buf, pnode_info);
                break; 
            case PEER_DEPARTURE_CMD:
                cmd_departure(tcp_resp_fd, buf, pnode_info);
                break;
            case PEER_KILL1_CMD:
                cmd_kill1(tcp_resp_fd, buf, pnode_info);
                break;
            case PEER_KILL2_CMD:
                cmd_kill2(tcp_resp_fd, buf, pnode_info);
                break;
            case DATA_INSERTION_CMD:
                cmd_insertion(tcp_resp_fd, buf, pnode_info);
                break;
            case DATA_RETRIEVAL_CMD:
                cmd_retrieval(tcp_resp_fd, buf, pnode_info);
                break;
            case INFORM_PREDC_JOIN_CMD:
                cmd_predc_join(tcp_resp_fd, buf, pnode_info);
                break;
            case INFORM_JOIN_CMD:
                cmd_inform_join(tcp_resp_fd, buf, pnode_info);
                break;
            case GET_FILE_CMD:
                cmd_get_file(tcp_resp_fd, buf, pnode_info);
            default:
                continue;
        }

        close(tcp_resp_join_fd);
    }
    
}

void cmd_join(int fd, char* buf, struct peer_node* pnode_info) {
    
    
    char insert_id  = *(buf+1);

    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    char second_succ = pnode_info->second_succ.port;
    char first_predc= pnode_info->first_predc.port;
    pthread_mutex_unlock(&pnode_info->mutex); // release lock

    int connect_peer;

    // printf("[resp] insert id %d\n", insert_id);
    // printf("[resp] first_succ %d\n", first_succ);
    // printf("[resp] peer_id %d\n", peer_id);

    // judge what kind of this this node is
    if ((insert_id < first_succ)||
            ((insert_id > peer_id) && (peer_id > first_succ))) {
        
        
        printf("[resp] Peer %d Join request received\n", insert_id);
        printf("[resp] My new first successor is Peer %d\n", insert_id);
        printf("[resp] My new second successor is Peer %d\n", first_succ);
        // acquire lock for structure pnode_info
        pthread_mutex_lock(&pnode_info->mutex); // add lock
        pnode_info->first_succ.port  = insert_id;
        pnode_info->second_succ.port = first_succ;
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
        

        // inform the joined node to add its successor
        connect_peer = insert_id;
        *buf     = (char) INFORM_JOIN_CMD;
        *(buf+1) = (char) first_succ; // pnode_info->first_succ.port 
        *(buf+2) = (char) second_succ;// pnode_info->second_succ.port
        tcp_req_handler(peer_id, connect_peer, buf);

        // inform first_predecessor to change its successor
        connect_peer = first_predc;
        *buf     = (char) INFORM_PREDC_JOIN_CMD;
        *(buf+1) = (char) insert_id;
        *(buf+2) = (char) 0;
        tcp_req_handler(peer_id, connect_peer, buf);
    } else {
        printf("Peer %d Join request forwarded to my successor\n", insert_id);
        connect_peer = first_succ;
        *buf     = (char) PEER_JOIN_CMD;
        *(buf+1) = (char) insert_id;
        tcp_req_handler(peer_id, connect_peer, buf);
    }
    
}

void cmd_departure(int fd, char* buf, struct peer_node* pnode_info) {
    
    // acquire lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    char second_succ = pnode_info->second_succ.port;
    char first_predc= pnode_info->first_predc.port;

    

    if (*(buf+1) == first_succ) {
        pnode_info->first_succ.port  = *(buf+2);
        pnode_info->second_succ.port = *(buf+3);
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock

        tcp_req_handler(peer_id, first_predc, buf);
        printf("Peer %d will depart from the network\n", *(buf+1));
        printf("My new first successor is Peer %d\n", *(buf+2));
        printf("My new first successor is Peer %d\n", *(buf+3));
    } else if (*(buf+1) == second_succ) {
        pnode_info->second_succ.port = *(buf+2);
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock

        printf("Peer %d will depart from the network\n", *(buf+1));
        printf("My new first successor is Peer %d\n", first_succ);
        printf("My new first successor is Peer %d\n", *(buf+2));
    }
}


void cmd_kill1(int fd, char* buf, struct peer_node* pnode_info){
    // acquire lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    // char second_succ = pnode_info->second_succ.port;
    // char first_predc= pnode_info->first_predc.port;
    
    if (*(buf+1) == (char) peer_id) { // port report the kill cmd
        pnode_info->first_succ.port  = pnode_info->second_succ.port;
        pnode_info->second_succ.port = *(buf+2);
        
        printf("Peer %d is no longer alive\n", first_succ);
        printf("My new first successor is Peer %d\n", pnode_info->first_succ.port);
        printf("My new second successor is Peer %d\n", pnode_info->second_succ.port);
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
    } else { // second successor
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
        *(buf+2) = (char) first_succ;
        tcp_req_handler(peer_id, *(buf+1), buf);
    }

    
}

void cmd_kill2(int fd, char* buf, struct peer_node* pnode_info) {
    // acquire lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    char second_succ = pnode_info->second_succ.port;

    if (*(buf+1) == (char) peer_id) {// port report the kill cmd
        pnode_info->second_succ.port = *(buf+2);
        printf("Peer %d is no longer alive\n", second_succ);
        printf("My new first successor is Peer %d\n", pnode_info->first_succ.port);
        printf("My new second successor is Peer %d\n", pnode_info->second_succ.port);
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
    } else { // first successor
        pthread_mutex_unlock(&pnode_info->mutex); // release write lock
        *(buf+2) = (char) first_succ;
        tcp_req_handler(peer_id, *(buf+1), buf);
    }
}

void cmd_insertion(int fd, char* buf, struct peer_node* pnode_info){
    // acquire lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    char first_predc= pnode_info->first_predc.port;
    pthread_mutex_unlock(&pnode_info->mutex); // release lock

    int file = (int) (*(buf+4) + (*(buf+5)<<8) + 
            (*(buf+6)<<16) + (*(buf+7)<<24)); // file name to get
    int store_port = file & 0xff;
    char file_str[5];
    int2str4(file, file_str);
    printf("%d-------------------\n", file);
    if (((first_predc < store_port)&&(store_port < peer_id)) ||
            ((first_predc < store_port)&&(first_predc > peer_id)) ||
            ((store_port < peer_id)&&(first_predc > peer_id)) ||
            (store_port == peer_id)) {
        printf("Store %s request accepted\n", file_str);
        return ;
    } else {
        // *(buf+1) = (int) file;
        printf("Store %s request forwarded to my successor\n", file_str);
        tcp_req_handler(peer_id, first_succ, buf);
    }
}

void cmd_retrieval(int fd, char* buf, struct peer_node* pnode_info){
    
    // acquire lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex); // add lock
    char peer_id    = pnode_info->self.port;
    char first_succ = pnode_info->first_succ.port;
    char first_predc= pnode_info->first_predc.port;
    pthread_mutex_unlock(&pnode_info->mutex); // release lock

    int connect_to = (char) *(buf+1); // port who needs the file
    int file = (int) (*(buf+4) + (*(buf+5)<<8) + 
            (*(buf+6)<<16) + (*(buf+7)<<24));
    int store_port = file & 0xff;
    char file_str[5];
    int2str4(file, file_str);
    if (((first_predc < store_port)&&(store_port < peer_id)) ||
            ((first_predc < store_port)&&(first_predc > peer_id)) ||
            ((store_port < peer_id)&&(first_predc > peer_id)) ||
            (store_port == peer_id)) {// condition for file store here 
        printf("File %s is stored here\n", file_str);
        printf("Sending file %s to Peer %d\n", file_str, connect_to);
        
        // param format
        *(buf)   = GET_FILE_CMD; // file tranmission state
        *(buf+1) = (char) peer_id;
        *(buf+4) = (char) file&0xff; // file name to fetch
        *(buf+5) = (char) (file>>8)&0xff; // file name to  fetch
        *(buf+6) = (char) (file>>16)&0xff; // file name to fetch
        *(buf+7) = (char) (file>>24)&0xff; // file name to fetch

        int conn_fd = tcp_req_establish(connect_to, buf);
        if (conn_fd < 0) {
            printf("tcp req establish failed\n");
            return;
        }
        send(conn_fd, buf, BUF_LEN, 0); // send cmd GET_FILE_CMD

        // printf("[resp] buf[0] %d\n", buf[0]);
        // printf("[resp] buf[1] %d\n", buf[1]);
        // printf("[resp] buf[2] %d\n", buf[2]);
        // printf("[resp] buf[3] %d\n", buf[3]);
        // printf("[resp] buf[4] %d\n", buf[4]);
        // printf("[resp] buf[5] %d\n", buf[5]);
        // printf("[resp] buf[6] %d\n", buf[6]);
        // printf("[resp] buf[7] %d\n", buf[7]);
        // printf("[cmd_retrieval] buf[8] %d\n", buf[8]);
        // printf("[cmd_retrieval] data received successfully!\n");
        char file_name[10];
        sprintf(file_name, "%s.pdf", file_str); 
        FILE* fp = fopen(file_name, "r");
        if (fp == NULL) {
            printf("[cmd_retrieval] fopen failed!\n");
        }
        *(buf) = 1; // used to indicate the end of transmition
        fgets(buf+1, sizeof(buf)-1, fp);
        printf("%s\n", buf);
        send(conn_fd, buf, BUF_LEN, 0);
        
        printf("The file has been sent\n");
        close(conn_fd);
        fclose(fp);
        return ;
    } else {
        printf("Request for File %d has been received, but the file is not stored here\n", file);
        tcp_req_handler(peer_id, first_succ, buf);
    }
}


void cmd_predc_join(int fd, char* buf, struct peer_node* pnode_info) {
    // acquire write lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex);
    pnode_info->second_succ.port = (char) *(buf+1);
    printf("Successor Change request received\n");
    printf("My new first successor is Peer %d\n", pnode_info->first_succ.port);
    printf("My new second successor is Peer %d\n", pnode_info->second_succ.port);
    pthread_mutex_unlock(&pnode_info->mutex); // release lock
}

void cmd_inform_join(int fd, char* buf, struct peer_node* pnode_info) {
    
    
    // acquire write lock for structure pnode_info
    pthread_mutex_lock(&pnode_info->mutex);
    pnode_info->block = 0;
    pnode_info->first_succ.port =  *(buf+1);
    pnode_info->second_succ.port =  *(buf+2);
    printf("Successor Change request received\n");
    printf("My new first successor is Peer %d\n", pnode_info->first_succ.port);
    printf("My new second successor is Peer %d\n", pnode_info->second_succ.port);
    pthread_mutex_unlock(&pnode_info->mutex); // release lock

}

void cmd_get_file(int fd, char* buf, struct peer_node* pnode_info) {
    
    
    char src_port = *(buf+1); // port which has the file
    // int file = *(buf+4); 
    int file = (int) (*(buf+4) + (*(buf+5)<<8) + 
            (*(buf+6)<<16) + (*(buf+7)<<24)); // file name

    char file_str[6];
    int2str4(file, file_str+1);
    file_str[0] = '_';
    printf("Peer %d had File %s\n", src_port, file_str+1);
    printf("Receiving File %s from Peer %d\n", file_str+1, src_port);
    FILE* fp = fopen(file_str, "w+");
    recv(fd, buf, BUF_LEN, 0);
    fprintf(fp, "%s", buf+1);
    // do {
    //     fprintf(fp, "%s", buf);
    //     recv(fd, buf, BUF_LEN, 0);
    // } while (*(buf) == 0);
    fflush(fp);
    fclose(fp);
    printf("File %s received\n", file_str+1);
}