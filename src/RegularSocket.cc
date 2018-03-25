// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>

#include <DMScu_FixedSizeString.h>

#include <RegularSocket.h>

// ----------------------------------------------------------------------------

int RegularSocket::send (const void *data, size_type the_size)  {

    int sent_size = 0;

    if (get_socket_type () == _stream_)    // TCP
        sent_size = ::send (get_fd (), data, the_size, MSG_NOSIGNAL);
    else    // UDP
        sent_size = ::send (get_fd (), data, the_size, MSG_CONFIRM);

    if (sent_size < 0)  {
        if (! is_blocking () && errno == EAGAIN)
            return (_try_again_);

        DMScu_FixedSizeString<2047> err;

        err.printf ("RegularSocket::send(): ::send(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (sent_size);
}

// ----------------------------------------------------------------------------

int RegularSocket::receive (void *data, size_type the_size)  {

    socklen_t   slug = 0;
    const   int received_size =
        get_socket_type () == _stream_

            ? ::recv (get_fd (), data, the_size, MSG_NOSIGNAL)
            : ::recvfrom (get_fd(), data, the_size, MSG_NOSIGNAL, NULL, &slug);

    if (received_size < 0)  {
        if (! is_blocking () && errno == EAGAIN)
            return (_try_again_);

        DMScu_FixedSizeString<2047> err;

        err.printf ("RegularSocket::receive(): ::recv(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (received_size);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
