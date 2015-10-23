#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <errno.h>
#include <string.h>

constexpr int port = 3100;

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
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
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(MasterSocket == -1)
    {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(port);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int Result = connect(MasterSocket , (struct sockaddr *)&SockAddr , sizeof(SockAddr));

    if(Result < 0)
    {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    //set_nonblock(MasterSocket);

    struct pollfd Set[1];
    Set[0].fd = MasterSocket;
    Set[0].events = POLLIN;

    while(true) {
        poll(Set, 1, -1);

        if (Set[0].revents & POLLIN) {
            static char Buffer[1024];
            int RecvSize = recv(MasterSocket, Buffer, 1024, MSG_NOSIGNAL);
            if (RecvSize > 0) {
                std::cout << Buffer;
            }
            if (fgets(Buffer, 1024, stdin))
                send(MasterSocket, Buffer, strlen(Buffer), MSG_NOSIGNAL);
        }
    }

    return 0;
}