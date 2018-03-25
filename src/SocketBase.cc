// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <cerrno>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <netinet/tcp.h>
#include <netdb.h>

extern  int h_errno;

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>

#include <SocketBase.h>

// ----------------------------------------------------------------------------

bool SocketBase::_connect_hook ()  {

    if (is_connected ())
        throw std::runtime_error("SocketBase::_connect_hook(): "
                                "Socket is already in connected state");

    fd_ = ::socket (get_ip_address_type () == _ipv6_ ? PF_INET6 : PF_INET,
                    get_socket_type () == _stream_ ? SOCK_STREAM : SOCK_DGRAM,
                    0);

    if (get_fd () < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("SocketBase::_connect_hook(): ::socket(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    struct  sockaddr_in addr_in;
    struct  ip_mreqn    ipm_addr;

    _get_sock_addr_in (addr_in, ipm_addr);

    if (get_socket_type () == _dgram_)    // UDP
        if (::setsockopt (get_fd (),
                          IPPROTO_IP,
                          IP_ADD_MEMBERSHIP,
                          &ipm_addr,
                          sizeof (ipm_addr)) < 0)  {
            close (get_fd ());

            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_connect_hook(): "
                        "::setsockopt(IP_ADD_MEMBERSHIP): (%d), %s, %d",
                        errno, strerror (errno), get_fd ());
            throw std::runtime_error(err.c_str ());
        }

    const   int on = 1;

    if (socket_rule_ == _server_)
        if (::setsockopt (get_fd (),
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          &on,
                          sizeof (on)) < 0)  {
            close (get_fd ());

            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_connect_hook(): "
                        "::setsockopt(SO_REUSEADDR): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

    if (get_socket_type () == _dgram_ || socket_rule_ == _server_)  {
       // this is to take care of a bug in some of Linux versions
       //
        addr_in.sin_addr.s_addr = INADDR_ANY;
        if (::bind (get_fd (),
                    reinterpret_cast<const struct sockaddr *>(&addr_in),
                    sizeof (struct sockaddr_in)) < 0)  {
            close (get_fd ());

            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_connect_hook(): "
                        "::bind(): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }

    if (get_orientation () == _connected_ && socket_rule_ == _client_)
        if (::connect (get_fd (),
                       reinterpret_cast<const struct sockaddr *>(&addr_in),
                       sizeof (struct sockaddr_in)) < 0)  {
            close (get_fd ());

            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_connect_hook(): "
                        "::connect(): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::_get_sock_addr_in (struct sockaddr_in &sai_addr,
                                    struct ip_mreqn &ipm_addr) const  {

    char    hostname [1024];

    if (hostname_.empty ())  {  // Local host is needed
        if (::gethostname (hostname, sizeof (hostname) - 1) < 0)  {
            close (get_fd ());

            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_get_sock_addr_in(): "
                        "::gethostname(): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }
    else
        ::strncpy (hostname, get_hostname (), sizeof (hostname) - 1);

    ::memset (&sai_addr, 0, sizeof (struct sockaddr_in));
    ::memset (&ipm_addr, 0, sizeof (struct ip_mreqn));

    in_addr_t               inet_a = 0;
    const   struct  hostent *ret_hostent = NULL;

    if (get_hostname_type () == _name_)  {
        ret_hostent = ::gethostbyname (hostname);

        if (ret_hostent == NULL)  {
            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_get_sock_addr_in(): "
                        "::gethostbyname(): (%d) %s",
                        h_errno, strerror (h_errno));
            throw std::runtime_error(err.c_str ());
        }
    }
    else  {  // _ip_address_
        // inet_a = ::inet_addr (hostname);

        struct  in_addr ina;

        ina.s_addr = -2;

        if (! inet_aton (hostname, &ina))  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("SocketBase::_get_sock_addr_in(): "
                        "::inet_aton(): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
        inet_a = ina.s_addr;
    }

    const   bool    got_addr = ret_hostent && ret_hostent->h_addr_list;

    if (get_hostname_type () == _name_ && ! got_addr)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("SocketBase::_get_sock_addr_in(): "
                    "::gethostbyname(): (%d) %s",
                    h_errno, strerror (h_errno));
        throw std::runtime_error(err.c_str ());
    }

    if (got_addr)  {
        ::memcpy (reinterpret_cast<char *>(&sai_addr.sin_addr.s_addr),
                  ret_hostent->h_addr_list [0],
                  sizeof (sai_addr.sin_addr.s_addr));

        ::memcpy (reinterpret_cast<char *>(&ipm_addr.imr_multiaddr.s_addr),
                  ret_hostent->h_addr_list [0],
                  sizeof (ipm_addr.imr_multiaddr.s_addr));
    }
    else  {
        sai_addr.sin_addr.s_addr = inet_a;
        ipm_addr.imr_multiaddr.s_addr = inet_a;
    }

    sai_addr.sin_family = get_ip_address_type() == _ipv6_ ? PF_INET6 : PF_INET;
    sai_addr.sin_port = htons (get_port ());

    ipm_addr.imr_address.s_addr = htonl (INADDR_ANY);
    ipm_addr.imr_ifindex = 0;

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::set_send_buffer_size (int buf_size)  {

    if (::setsockopt (get_fd (),
                      SOL_SOCKET,
                      SO_SNDBUF,
                      reinterpret_cast<const void *>(&buf_size),
                      sizeof (buf_size)) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("SocketBase::set_send_buffer_size(): "
                    "::setsockopt(SNDBUF): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::set_receive_buffer_size (int buf_size)  {

    if (::setsockopt (get_fd (),
                      SOL_SOCKET,
                      SO_RCVBUF,
                      reinterpret_cast<const void *>(&buf_size),
                      sizeof (buf_size)) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("SocketBase::set_receive_buffer_size(): "
                    "::setsockopt(SO_RCVBUF): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (true);
}

// ----------------------------------------------------------------------------

int SocketBase::get_free_send_buffer_space ()  {

    int         buf_size = 0;
    socklen_t   t_size = sizeof (buf_size);

    if (::getsockopt (get_fd (),
                      SOL_SOCKET,
                      SO_SNDBUF,
                      reinterpret_cast<void *>(&buf_size),
                      &t_size) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("SocketBase::get_free_send_buffer_space (): "
                    "::getsockopt(SNDBUF): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    int used_buf_size = 0;

    if (::ioctl (get_fd (), TIOCOUTQ, &used_buf_size) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("SocketBase::get_free_send_buffer_space (): "
                    "::ioctl(TIOCOUTQ): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (buf_size - used_buf_size);
}

// ----------------------------------------------------------------------------

// Disable Nagel's algorithm.
// This means that segments are always sent as soon as possible,
// even if there is only a small amount of data.
//
bool SocketBase::disable_nagel_algorithm ()  {

    const   int on = 1;

    if (::setsockopt (get_fd (),
                      SOL_TCP,
                      TCP_NODELAY,
                      &on,
                      sizeof (on)) < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("SocketBase::_connect_hook(): "
                    "::setsockopt(TCP_NODELAY): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::_disconnect_hook ()  {

    if (get_socket_type () == _dgram_)  {  // UDP
        struct  sockaddr_in addr_in;
        struct  ip_mreqn    ipm_addr;

        _get_sock_addr_in (addr_in, ipm_addr);

        ::setsockopt (get_fd (),
                      SOL_IP,
                      IP_DROP_MEMBERSHIP,
                      &ipm_addr,
                      sizeof (ipm_addr));
    }

    close (get_fd ());

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::_make_nonblocking_hook ()  {

    if (is_blocking ())  {
        int cmd_value = 0;

        if ((cmd_value = ::fcntl (get_fd (), F_GETFL, &cmd_value)) < 0)  {
            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_make_nonblocking_hook(): "
                        "::fcntl(F_GETFL): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

        cmd_value |= O_NONBLOCK;
        if (::fcntl (get_fd (), F_SETFL, cmd_value) < 0)  {
            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_make_nonblocking_hook(): "
                        "::fcntl(F_SETFL): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::_make_blocking_hook ()  {

    if (! is_blocking ())  {
        int cmd_value = 0;

        if ((cmd_value = ::fcntl (get_fd (), F_GETFL, &cmd_value)) < 0)  {
            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_make_blocking_hook(): "
                        "::fcntl(F_GETFL): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

        cmd_value &= ~O_NONBLOCK;
        if (::fcntl (get_fd (), F_SETFL, cmd_value) < 0)  {
            DMScu_FixedSizeString<2047> err;

            err.printf ("SocketBase::_make_blocking_hook(): "
                        "::fcntl(F_SETFL): (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }

    return (true);
}

// ----------------------------------------------------------------------------

SocketBase::SELECT_RESULT SocketBase::
select (int operation, long seconds, long mseconds)  {

    fd_set    readfds;
    fd_set    writefds;
    fd_set    errorfds;

    FD_ZERO (&readfds);
    FD_ZERO (&writefds);
    FD_ZERO (&errorfds);

    if (operation & _read_)
        FD_SET (get_fd (), &readfds);
    if (operation & _write_)
        FD_SET (get_fd (), &writefds);
    if (operation & _error_)
        FD_SET (get_fd (), &errorfds);

    timeval tval;

    tval.tv_sec = seconds;
    tval.tv_usec = mseconds;

    const   int rc =
        ::select (get_fd () + 1, &readfds, &writefds, &errorfds, &tval);

    if (rc < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("SocketBase::select(): ::select(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    if (rc == 0)
        return (_timedout_);
    if (FD_ISSET (get_fd (), &errorfds))
        return (_exception_);
    if (FD_ISSET (get_fd (), &writefds) && FD_ISSET (get_fd (), &readfds))
        return (_rw_ready_);
    if (FD_ISSET (get_fd (), &writefds))
        return (_write_ready_);
    if (FD_ISSET (get_fd (), &readfds))
        return (_read_ready_);
}

// ----------------------------------------------------------------------------

// get_peer_host (DMScu_VirtualString &hostname, in_port_t &port) const {
bool SocketBase::get_peername_by_fd (int fd,
                                            DMScu_VirtualString &host_name,
                                            in_port_t &port,
                                            IP_ADDRESS_TYPE ip_at)  {

    struct  sockaddr_in client_addr;
    socklen_t           client_addr_size = sizeof (struct sockaddr_in);

    if (::getpeername (fd,
                       reinterpret_cast<struct sockaddr *>(&client_addr),
                       &client_addr_size) < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("SocketBase::get_peername_by_fd(): "
                    "::getpeername(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    const   struct  hostent *host =
        ::gethostbyaddr (reinterpret_cast<const char *>(&client_addr.sin_addr),
                         sizeof (client_addr.sin_addr),
                         ip_at == _ipv6_ ? PF_INET6 : PF_INET);

    port = ntohs (client_addr.sin_port);
    if (host == NULL || host->h_name == NULL)
        host_name = ::inet_ntoa (client_addr.sin_addr);
    else
        host_name = host->h_name;

    return (true);
}

// ----------------------------------------------------------------------------

bool SocketBase::get_hostname_by_ip (const char *dotnotation_ip,
                                            DMScu_VirtualString &host_name,
                                            DMScu_VirtualString &server_name,
                                            IP_ADDRESS_TYPE ip_at)  {

    struct  sockaddr_in sa;

    ::memset (&sa, 0, sizeof (sa));
    ::inet_pton (ip_at == _ipv4_ ? AF_INET : AF_INET6,
                 dotnotation_ip,
                 &sa.sin_addr); 
    sa.sin_family = ip_at == _ipv4_ ? PF_INET : PF_INET6;

    char    node [NI_MAXHOST];
    char    server [NI_MAXSERV];

    *node = 0;
    *server = 0;

    const   int res =
        ::getnameinfo (reinterpret_cast<struct sockaddr *>(&sa), sizeof (sa),
                       node, sizeof (node),
                       server, sizeof (server),
                       NI_NAMEREQD);

    if (res == 0)  {
        host_name = node;
        server_name = server;
    }

    return (true);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
