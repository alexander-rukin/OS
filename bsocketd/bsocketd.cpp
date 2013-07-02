#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>

struct linked_list {
    std::string value;
    int count_links;
    linked_list* next;
    linked_list(std::string value, int count_links) :
    value(value), count_links(count_links), next(NULL) {}
};

struct m_queue {
    int count_heads;
    linked_list* head;
    linked_list* tail;
    std::map<int, linked_list*> heads;
    m_queue() {
        count_heads = 0;
        head = tail = new linked_list("", 0);
    }
    
    void add_head(int fd) {
        count_heads++;
        heads[fd] = tail;
        tail->count_links++;
    }
    
    bool empty(int fd) {
        return heads[fd]->next == NULL;
    }
    
    void push(std::string s) {
        if (head == NULL) {
            head = new linked_list(s, count_heads);
            tail = head;
        } else {
            linked_list* el = new linked_list(s, 0);
            tail->next = el;
            tail = el;
        }
    }
    
    std::string get(int fd) {
        linked_list* cur = heads[fd];
        heads[fd] = cur->next;
        cur->count_links--;
        if (cur == head && cur->count_links == 0) {
            head = head->next;
            delete cur;
            return head->value;
        }
        return cur->next->value;
    }
};

char *my_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL)
        exit(EXIT_FAILURE);
    return static_cast<char*>(ptr);
}

int read_fd(int fd, char *buf, size_t len) {
    int read_count = read(fd, buf, len);
    if (read_count < 0)
        exit(EXIT_FAILURE);
    return read_count;
}

int my_poll(pollfd fds[], int nfds, int timeout) {
    int count = poll(fds, nfds, timeout);
    if (count == -1)
        exit(EXIT_FAILURE);
    return count;
}

pid_t pid;

void handler(int) {
    kill(pid, SIGINT);
}

void handler2(int) {}

std::string int_to_str(int val) {
    std::string res = "";
    while (val > 0) {
        char k = val % 10 + '0';
        res += k;
        res += res;
        val /= 10;
    }
    return res;
}

int main (int argc, char** argv) {

    if(argc < 2) {
        printf("Usage: %s port\n",argv[0]);
        return 2;
    }    

    pid = fork();
    if (pid) {
        signal(SIGINT, handler);
        int status;
        waitpid(pid, &status, 0);
        printf("\nGlobal disconnect\n");
        return 0;
    }
    setsid();
    addrinfo hints;
    
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    
    addrinfo *result;
    if (getaddrinfo(NULL, argv[1], &hints, &result) != 0)
        exit(EXIT_FAILURE);

    if (result == NULL)
        exit(EXIT_FAILURE);
    
    int socket_fd;
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1)
        exit(EXIT_FAILURE);
    
    int sso_status = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sso_status, sizeof(int)) == -1)
        exit(EXIT_FAILURE);
    
    if (bind(socket_fd, result->ai_addr, result->ai_addrlen) == -1)
        exit(EXIT_FAILURE);
    
    const int backlog = 5;
    if (listen(socket_fd, backlog) == -1)
        exit(EXIT_FAILURE);
    
    pollfd fd[backlog + 1];
    fd[0].fd = socket_fd;
    fd[0].events = POLLIN;
    std::string read_buffers[backlog];
    std::string buf_sizes[backlog];
    int i_buf_sizes[backlog];
    std::string write_buffers[backlog];
    for (size_t i = 0; i < backlog; i++) {
        read_buffers[i] = "";
        buf_sizes[i] = "";
        i_buf_sizes[i] = 0;
        write_buffers[i] = "";
    }
    
    const size_t buf_len = 1024;
    const int timeout = -1;
    int clients = 1;
    
    char* buf = my_malloc(buf_len);
    signal(SIGHUP, handler2);
    m_queue q;
    std::string claddr[backlog];
    while (true) {
        my_poll(fd, clients, timeout);
        for (int i = 1; i < clients; i++) {
            if (fd[i].revents & (POLLERR | POLLHUP)) {
                std::cout << "Client #" << fd[i].fd << " disconnected" << std::endl;
                close(fd[i].fd);
                fd[i] = fd[clients - 1];
                read_buffers[i - 1] = read_buffers[clients - 2];
                buf_sizes[i - 1] = buf_sizes[clients - 2];
                write_buffers[i - 1] = write_buffers[clients - 2];
                clients--;
                continue;
            }
            if (fd[i].revents & POLLIN) {
                int cur_buf_size = i_buf_sizes[i - 1];
                if (read_buffers[i - 1].length() < cur_buf_size) {
                    int read_count = read_fd(fd[i].fd, buf,
                            cur_buf_size - read_buffers[i - 1].length());
                    for (int j = 0; j < read_count; j++)
                        read_buffers[i - 1] += buf[j];
                } else {
                    int read_count = read_fd(fd[i].fd, buf, 4 - 
                            buf_sizes[i - 1].length());
                    for (int j = 0; j < read_count; j++)
                        buf_sizes[i - 1] += buf[j];
                    if (buf_sizes[i - 1].length() == 4) {
                        int len = atoi(buf_sizes[i - 1].c_str());
                        read_count = read_fd(fd[i].fd, buf, len);
                        std::string mes = "";
                        for (int j = 0; j < read_count; j++)
                            mes += buf[j];
                        i_buf_sizes[i - 1] = len;
                        read_buffers[i - 1] = mes;
                    }
                }
                if (read_buffers[i - 1].length() == i_buf_sizes[i - 1]) {
                    q.push(claddr[i - 1] + ' ' + read_buffers[i - 1]);
                    read_buffers[i - 1] = "";
                    buf_sizes[i - 1] = "";
                    i_buf_sizes[i - 1] = 0;
                }
            }
            if (fd[i].revents & POLLOUT) {
                if (q.empty(fd[i].fd) && write_buffers[i - 1] == "")
                    continue;
            
                if (write_buffers[i - 1] == "") {
                     std::string mes = q.get(fd[i].fd);
                     write_buffers[i - 1] = mes;
                }
                int write_count = write(fd[i].fd, 
                    (write_buffers[i - 1]).c_str(), 
                    write_buffers[i - 1].length());
                write_buffers[i - 1] =  write_buffers[i - 1].substr(write_count);
            }
            if (!q.empty(fd[i].fd))
                fd[i].events |= POLLOUT;
            else
                fd[i].events &= (~POLLOUT);
        }
	    if (fd[0].revents & POLLIN) {
            sockaddr_in client;
            client.sin_family = AF_INET;
            socklen_t addr_size = sizeof(client);
            int fd_acc = accept(socket_fd,
                                (struct sockaddr *) &client, &addr_size);
            if (fd_acc == -1)
                exit(EXIT_FAILURE);
            claddr[clients - 1] = inet_ntoa(client.sin_addr);
            fd[clients].fd = fd_acc;
            fd[clients].events = POLLIN | POLLOUT;
            q.add_head(fd_acc);
            std::cout << "New client #" << fd[clients].fd << std::endl;
            clients += 1;
	    }

    }
    return 0;
}
