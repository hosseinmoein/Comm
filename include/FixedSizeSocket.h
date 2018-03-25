// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <SocketBase.h>

// ----------------------------------------------------------------------------

class   FixedSizeSocket : public SocketBase  {

    public:

        class   SocketWriteDetail  {

            public:

                inline SocketWriteDetail () throw ()
                    : msg_sent (0), has_hdr_sent (false), hdr_sent (0)  {   }

                inline explicit SocketWriteDetail (size_type msg_sent_val)
                    throw ()
                    : msg_sent (msg_sent_val),
                      has_hdr_sent (false),
                      hdr_sent (0)  {   }

                inline SocketWriteDetail (size_type msg_sent_val,
                                          size_type hdr_sent_val) throw ()
                    : msg_sent (msg_sent_val),
                      has_hdr_sent (true),
                      hdr_sent (hdr_sent_val)  {   }

                size_type   msg_sent;
                bool        has_hdr_sent;
                size_type   hdr_sent;
        };

        typedef SocketBase   BaseClass;
        typedef unsigned char       ReadBufferType;

        static  const   size_type   MAX_FAST_HEADER_SIZE;

        inline FixedSizeSocket (const char *name,
                                       IP_ADDRESS_TYPE ip_address_type,
                                       SOCKET_TYPE socket_type,
                                       SOCKET_RULE socket_rule,
                                       int port,
                                       HOSTNAME_TYPE hostname_type = _name_,
                                       const char *hostname = NULL,
                                       ORIENTATION orientation = _connected_,
                                       bool use_fast_proto = false,
                                       size_type header_size = 10) throw ()
            : BaseClass (name,
                         ip_address_type,
                         socket_type,
                         socket_rule,
                         port,
                         hostname_type,
                         hostname,
                         orientation),
              use_fast_ (use_fast_proto),
              header_size_ (header_size)  {   }

        inline virtual ~FixedSizeSocket ()  {   }

        size_type write (const void *data,
                         size_type the_size,
                         const SocketWriteDetail *already_sent = NULL,
                         SocketWriteDetail *write_detail = NULL);
        size_type read (ReadBufferType **data, bool text_data = false);
        size_type read_fixed (ReadBufferType *data, bool text_data = false);

       // If the_size bytes are available in the socket buffer a true
       // is returned, otherwise a false is returned. The actual data is
       // not removed from the socket buffer.
       //
        bool peek (size_type the_size, void *buffer) throw ();

        inline size_type
        get_header_size () const throw ()  { return (header_size_); }

    protected:

        virtual int send (const void *data, size_type the_size);
        virtual int receive (void *data, size_type the_size);

        size_type _compute_size_for_read ();

    private:

        const   bool        use_fast_;
        const   size_type   header_size_;
};

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
