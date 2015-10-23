#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

#include <iostream>
#include <algorithm>
#include <set>
#include <map>

#include <sys/epoll.h>
#include <system_error>
#include <string.h>

constexpr int MAX_EVENTS = 32;
constexpr int port = 3100;
constexpr char hello_msg[] = "Welcome, stranger!\n";
const size_t hello_len = std::char_traits<char>::length(hello_msg);

using namespace std::string_literals;
using std::string;
using std::to_string;

int set_nonblock(int fd)
{
    int flags;
#ifdef O_NONBLOCK
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
}

int main(int argc, char **argv)
{
    //FileLogger myLog ("log.txt");
    std::ostream &myLog = std::cout;

    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(MasterSocket == -1)
    {
        myLog << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(port);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    int Result = bind(MasterSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr));

    if(Result == -1)
    {
        myLog << strerror(errno) << std::endl;
        return 1;
    }
    myLog << "Created master socket on port " << port << std::endl;

    set_nonblock(MasterSocket);

    Result = listen(MasterSocket, SOMAXCONN);

    if(Result == -1)
    {
        myLog << strerror(errno) << std::endl;
        return 1;
    }

    struct epoll_event Event;
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN;

    struct epoll_event * Events;
    Events = (struct epoll_event *) calloc(MAX_EVENTS, sizeof(struct epoll_event));

    int EPoll = epoll_create1(0);
    epoll_ctl(EPoll, EPOLL_CTL_ADD, MasterSocket, &Event);

    std::set<int> Clients;
    struct timeval tv;
    tv.tv_sec = 16;
    tv.tv_usec = 0;

    std::map<int, string> message_parts;

    while(true)
    {
        int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
        for (unsigned i = 0; i < N; ++i)
        {
            if (Events[i].data.fd == MasterSocket)
            {
                int SlaveSocket = accept(MasterSocket, 0, 0);
                if (SlaveSocket < 0)
                    throw std::system_error(errno, std::system_category());
                if (set_nonblock(SlaveSocket) < 0)
                    throw std::system_error(errno, std::system_category());

                myLog << "accepted connection (id " << SlaveSocket << ')' << std::endl;
                ssize_t SendSize = send(SlaveSocket, hello_msg, hello_len, 0);
                if (SendSize < 0)
                    myLog << "Send error. " << strerror(errno) << std::endl;
                Clients.insert(SlaveSocket);

                struct epoll_event Event;
                Event.data.fd = SlaveSocket;
                Event.events = EPOLLIN;

                epoll_ctl(EPoll, EPOLL_CTL_ADD, SlaveSocket, &Event);
            }
            else {
                static char Buffer[1024];
                ssize_t RecvSize = recv(Events[i].data.fd, Buffer, 1024, MSG_NOSIGNAL);
                if (RecvSize > 0) {
                    Buffer[RecvSize] = '\0';
                    message_parts[Events[i].data.fd] += Buffer;
                    size_t pos;
                    while ((pos = message_parts[Events[i].data.fd].find("\n")) != string::npos) {
                        string curr_msg = message_parts[Events[i].data.fd].substr(0, pos + 1);
                        message_parts[Events[i].data.fd].erase(0, pos + 1);
                        string send_msg = "Message from "s + to_string(Events[i].data.fd) + ": " + curr_msg;
                        myLog << send_msg;
                        for (auto fd: Clients) {
                            send(fd, send_msg.c_str(), send_msg.size(), MSG_NOSIGNAL);
                        }
                    }
                }
                if (RecvSize <= 0) {
                    if (RecvSize < 0)
                        perror("recv");
                    myLog << "connection terminated (id " << Events[i].data.fd << ')' << std::endl;
                    message_parts.erase(Events[i].data.fd);
                    Clients.erase(Events[i].data.fd);
                    shutdown(Events[i].data.fd, SHUT_RDWR);
                    close(Events[i].data.fd);
                }
            }
        }
    }

    return 0;
}