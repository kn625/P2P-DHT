
#include "utils.h"



////////////////////////////////////////////////////////////////////////
// Wrappers for sendto and recv

// A wrapper for the recvfrom function.
// The `who` parameter will be "send" or "recv", to make the output
// clearer (so that you can see which thread called the function).
int recv_wrapper(int fd, char *buf, struct sockaddr_in *sockaddr_from,
        socklen_t *from_len, char *who) {

    char name[BUF_LEN] = {0};

    // printf("[%s] Receiving...\n", who);
    int numbytes = recvfrom(fd, buf, BUF_LEN, 0,
        (struct sockaddr *) sockaddr_from, from_len);

    buf[numbytes] = '\0';
    buf[strcspn(buf, "\n")] = '\0';

    struct peer_info client = get_peer_info(sockaddr_from);

    // printf("[%s] Received %d bytes from %s: %s\n", who, numbytes,
    //         print_peer_buf(client, name), buf);

    return numbytes;
}

// A wrapper for the sendto function.
// The `who` parameter will be "send" or "recv", to make the output
// clearer (so that you can see which thread called the function).
int send_wrapper(int fd, char *buf, struct sockaddr_in *sockaddr_to,
        socklen_t *to_len, char *who) {

    char name[BUF_LEN] = {0};

    int numbytes = sendto(fd, buf, strlen(buf), 0,
            (struct sockaddr *) sockaddr_to, *to_len);

    struct peer_info client = get_peer_info(sockaddr_to);

    // printf("[%s] Sent %d bytes to %s: %s\n", who, numbytes,
    //         print_peer_buf(client, name), buf);

    return numbytes;

}
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// Socket helper functions


// Wrapper function for fgets, similar to Python's built-in 'input'
// function.
void get_input (char *buf, char *msg) {
    printf("%s", msg);
    fgets(buf, BUF_LEN, stdin);
    buf[strcspn(buf, "\n")] = '\0'; // Remove the newline
}

// Populate a `sockaddr_in` struct with the IP / port to connect to.
void fill_sockaddr(struct sockaddr_in *sa, char *ip, int port) {

    // Set all of the memory in the sockaddr to 0.
    memset(sa, 0, sizeof(struct sockaddr_in));

    // IPv4.
    sa->sin_family = AF_INET;

    // We need to call `htons` (host to network short) to convert the
    // number from the "host" endianness (most likely little endian) to
    // big endian, also known as "network byte order".
    sa->sin_port = htons(port);
    inet_pton(AF_INET, ip, &(sa->sin_addr));
}


// Populates a peer_info struct from a `sockaddr_in`.
struct peer_info get_peer_info(struct sockaddr_in *sa) {
    struct peer_info info = {};
    info.port = ntohs(sa->sin_port);
    inet_ntop(sa->sin_family, &(sa->sin_addr), info.host, INET_ADDRSTRLEN);

    return info;
}

// Get the "name" (IP address) of who we're communicating with.
// Takes in an array to store the name in.
// Returns a pointer to that array for convenience.
// char *get_name(struct sockaddr_in *sa, char *name) {
//     inet_ntop(sa->sin_family, &(sa->sin_addr), name, BUF_LEN);
//     return name;
// }

// Compare two client_info structs to see whether they're the same.
int peers_equal(struct peer_info a, struct peer_info b) {
    return (a.port == b.port+PORT_BASE && !strcmp(a.host, b.host));
}

// Get the current date/time.
char *get_time(void) {
    time_t t = time(0);
    return ctime(&t);
}

void print_peer(struct peer_info peer) {
    printf("%s:%d\n", peer.host, peer.port);
}

char *print_peer_buf(struct peer_info peer, char *buf) {
    sprintf(buf, "%s:%d", peer.host, peer.port);
    return buf;
}

int int2str4(int num, char* num4_str) {
    if (num < 10) {
        sprintf(num4_str, "000%d", num);
    } else if (num < 100) {
        sprintf(num4_str, "00%d", num);
    } else if (num < 1000) {
        sprintf(num4_str, "0%d", num);
    } else if (num < 10000) {
        sprintf(num4_str, "%d", num);
    } else {
        return -1;
    }
    return 0;
}


int is_quit(char* input) {
    char quit[5] = "Quit";
    for (int i = 0; i < 4; ++i) {
        if (input[i]!=quit[i]) {
            return 0;
        }
    }
    return 1;
}

int is_store(char* input) {
    char quit[6] = "Store";
    for (int i = 0; i < 5; ++i) {
        if (input[i]!=quit[i]) {
            return 0;
        }
    }
    return 1;
}

int is_request(char* input) {
    char quit[8] = "Request";
    for (int i = 0; i < 7; ++i) {
        if (input[i]!=quit[i]) {
            return 0;
        }
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
