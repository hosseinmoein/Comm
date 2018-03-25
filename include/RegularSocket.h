// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <SocketBase.h>

// ----------------------------------------------------------------------------

class   RegularSocket : public SocketBase  {

    public:

        typedef SocketBase   BaseClass;

        inline RegularSocket (const char *name,
                                     IP_ADDRESS_TYPE ip_address_type,
                                     SOCKET_TYPE socket_type,
                                     SOCKET_RULE socket_rule,
                                     int port,
                                     HOSTNAME_TYPE hostname_type = _name_,
                                     const char *hostname = "",
                                     ORIENTATION orientation = _connected_)
            throw ()
            : BaseClass (name,
                         ip_address_type,
                         socket_type,
                         socket_rule,
                         port,
                         hostname_type,
                         hostname,
                         orientation)  {   }
        inline virtual ~RegularSocket ()  {   }

        virtual int send (const void *data, size_type the_size);
        virtual int receive (void *data, size_type the_size);
};

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
