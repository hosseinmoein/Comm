// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>

#include <DMScu_FixedSizeString.h>
#include <DMScu_FASTProtocolUtilities.h>

#include <FixedSizeSocket.h>

// ----------------------------------------------------------------------------

const   FixedSizeSocket::size_type  FixedSizeSocket::
    MAX_FAST_HEADER_SIZE =
        DMScu_FASTProtocolUtilities::bytes_required (sizeof(size_type), false);

// ----------------------------------------------------------------------------

bool FixedSizeSocket::
peek (size_type the_size, void *buffer) throw ()  {

    const   size_type   recved_size =
        ::recv (get_fd (), buffer, the_size, MSG_PEEK);

    return (recved_size == the_size);
}

// ----------------------------------------------------------------------------

int FixedSizeSocket::send (const void *data, size_type the_size)  {

    const   int sent_size =
        get_socket_type () == _stream_  // TCP
            ? ::send (get_fd (), data, the_size, MSG_NOSIGNAL)  // TCP
            : ::send (get_fd (), data, the_size, MSG_CONFIRM);  // UDP

    if (sent_size < 0)  {
        if (! is_blocking () && errno == EAGAIN)
            return (_try_again_);

        DMScu_FixedSizeString<2047> err;

        err.printf ("FixedSizeSocket::send(): ::send(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (sent_size);
}

// ----------------------------------------------------------------------------

int FixedSizeSocket::receive (void *data, size_type the_size)  {

    const   int recved_size =
        ::recv (get_fd (), data, the_size, MSG_WAITALL | MSG_NOSIGNAL);

    if (recved_size < 0)  {
        if (! is_blocking () && errno == EAGAIN)
            return (_try_again_);

        DMScu_FixedSizeString<2047> err;

        err.printf ("FixedSizeSocket::receive(): ::recv(): (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }
    else if (recved_size < the_size)  {
        DMScu_FixedSizeString<2047> err;

        err.printf ("FixedSizeSocket::receive(): ::recv(): "
                    "returned only %u bytes of %u requested",
                    recved_size, the_size);
        throw std::runtime_error(err.c_str ());
    }

    return (recved_size);
}

// ----------------------------------------------------------------------------

FixedSizeSocket::size_type
FixedSizeSocket::write (const void *data,
                               size_type the_size,
                               const SocketWriteDetail *already_sent,
                               SocketWriteDetail *write_detail)  {

    if (the_size != 0 && data == NULL)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("FixedSizeSocket::write(): "
                    "data pointer is NULL and size is %lu.", the_size);
        throw std::runtime_error(err.c_str ());
    }

    const   bool    has_hdr_sent =
        (already_sent ? already_sent->has_hdr_sent : false);
    size_type       hdr_sent = has_hdr_sent ? already_sent->hdr_sent : 0;

    if (use_fast_ && ! has_hdr_sent)  {
        unsigned    char    buffer [MAX_FAST_HEADER_SIZE];
        const   size_type   header_size =
            DMScu_FASTProtocolUtilities::encode_uinteger (buffer, the_size);

        if (! is_blocking () &&
            get_free_send_buffer_space () < the_size + header_size)  {
            if (write_detail)
                *write_detail = SocketWriteDetail (0);

            return (0);
        }

        if (send (buffer, header_size) < header_size)  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("FixedSizeSocket::write(): ::send() msg size: "
                        "could not send msg size");
            throw std::runtime_error(err.c_str ());
        }

        hdr_sent = header_size;
        if (write_detail)
            *write_detail = SocketWriteDetail (0, hdr_sent);
    }
    else if (! use_fast_ && (hdr_sent < get_header_size ()))  {
        if (! has_hdr_sent && ! is_blocking () &&
            get_free_send_buffer_space () < the_size + get_header_size ())  {
            if (write_detail)
                *write_detail = SocketWriteDetail (0);

            return (0);
        }

        char    buffer [get_header_size ()];

        ::sprintf (buffer, "%d", the_size);

        hdr_sent += send (buffer + hdr_sent, get_header_size () - hdr_sent);
        if (write_detail)
            *write_detail = SocketWriteDetail (0, hdr_sent);

        if (hdr_sent < get_header_size ())
            return (0);
    }

    const   int sent_size = the_size > 0 ? send (data, the_size) : 0;

    if (is_blocking () && sent_size <= 0 && the_size > 0) {
        DMScu_FixedSizeString<2047> err;

        err.printf ("FixedSizeSocket::write(): ::send(): "
                    "could not send msg");
        throw std::runtime_error(err.c_str ());
    }

    if (write_detail)
        write_detail->msg_sent = sent_size;

    return (sent_size);
}

// ----------------------------------------------------------------------------

FixedSizeSocket::size_type
FixedSizeSocket::_compute_size_for_read ()  {

    if (use_fast_)  {
        unsigned    char    buffer [MAX_FAST_HEADER_SIZE];
        unsigned    char    *tmp = buffer;
        size_type           rec_size = 0;

        do  {
            if (receive (tmp++, 1) != 1)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("FixedSizeSocket::"
                            "_compute_size_for_read(): "
                            "Could not read message size.  "
                            "Read 0 bytes now and %u so far.",
                            rec_size);
                throw std::runtime_error(err.c_str ());
            }

            rec_size += 1;
            if (rec_size > MAX_FAST_HEADER_SIZE)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("FixedSizeSocket::"
                            "_compute_size_for_read(): "
                            "Header was too big.  Expected max %lu; "
                            "read %lu bytes so far.",
                            MAX_FAST_HEADER_SIZE, rec_size);
                throw std::runtime_error(err.c_str ());
            }
        } while (! DMScu_FASTProtocolUtilities::is_final (*(tmp - 1)));

        size_type   the_size = 0;

        DMScu_FASTProtocolUtilities::decode_uinteger (buffer, the_size);

        return (the_size);
    }
    else  {
        char        buffer [header_size_ + 1];
        const   int rec_size = receive (buffer, get_header_size ());

        if (rec_size < get_header_size ())  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("FixedSizeSocket::_compute_size_for_read(): "
                        "Expected to get %lu bytes but read %d bytes.",
                        get_header_size (), rec_size);
            throw std::runtime_error(err.c_str ());
        }

        buffer [get_header_size ()] = 0;
        errno = 0;

        const   long    long    int msg_size = ::strtoll (buffer, NULL, 0);

        if (msg_size >= ULONG_MAX || msg_size < 0 || errno > 0)  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("FixedSizeSocket::_compute_size_for_read(): "
                        "::strtol('%s'): (%d) %s",
                        buffer, errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

        return (static_cast<size_type>(msg_size));
    }
}

// ----------------------------------------------------------------------------

FixedSizeSocket::size_type
FixedSizeSocket::read (ReadBufferType **data, bool text_data)  {

    const   size_type   the_size = _compute_size_for_read ();

    if (the_size > 0)  {
        *data = new ReadBufferType [text_data ? the_size + 1 : the_size];

        try  {
            receive (*data, the_size);
            if (text_data)
                (*data) [the_size] = 0;
        }
        catch (...)  {
            delete[] *data;
            *data = NULL;
            throw;
        }
    }
    else
        *data = NULL;

    return (the_size);
}

// ----------------------------------------------------------------------------

// data buffer must be large enough to take the message + the optional NULL
// terminator. In many cases (sending a single tick), the data size is limited,
// so this should be safe
//
FixedSizeSocket::size_type
FixedSizeSocket::read_fixed (ReadBufferType *data, bool text_data)  {

    const   size_type   the_size = _compute_size_for_read ();

    if (the_size > 0)  {
        receive (data, the_size);
        if (text_data)
            data [the_size] = 0;
    }

    return (the_size);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
