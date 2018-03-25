// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <cerrno>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>

#include <DMScu_FixedSizeString.h>

#include <Acceptor.h>

// ----------------------------------------------------------------------------

template<class com_BASE>
bool Acceptor<com_BASE>::listen (typename BaseClass::size_type qsize)  {

    if (::listen (BaseClass::get_fd (), qsize) < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("Acceptor::accept(): ::listen(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

   return (true);
}

// ----------------------------------------------------------------------------

template<class com_BASE>
com_BASE *Acceptor<com_BASE>::accept ()  {

    struct  sockaddr_in addr_in;
    struct  ip_mreqn    ipm_addr;

    BaseClass::_get_sock_addr_in (addr_in, ipm_addr);

    socklen_t    addr_len = sizeof (addr_in);
    const    int new_fd =
        ::accept (BaseClass::get_fd (),
                  reinterpret_cast<struct ::sockaddr *> (&addr_in),
                  &addr_len);

    if (new_fd < 0)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("Acceptor::accept(): ::accept(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    BaseClass    *ret_ptr = new BaseClass (BaseClass::get_name (),
                                           BaseClass::_ipv4_,
                                           BaseClass::_stream_,
                                           BaseClass::_client_,
                                           BaseClass::get_port (),
                                           BaseClass::get_hostname_type (),
                                           BaseClass::get_hostname ());

    ret_ptr->set_fd (new_fd);
    ret_ptr->set_connected (true);

    return (ret_ptr);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
