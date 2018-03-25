// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <netinet/in.h>

// ----------------------------------------------------------------------------

template<class com_BASE>
class   Acceptor : public com_BASE  {

    public:

        typedef com_BASE    BaseClass;

        inline Acceptor (
          const char *name,
          in_port_t port,
          typename BaseClass::HOSTNAME_TYPE hostname_type = BaseClass::_name_,
          const char *hostname = NULL) throw ()
            : BaseClass (name, BaseClass::_ipv4_, BaseClass::_stream_,
                         BaseClass::_server_, port, hostname_type,
                         hostname)  {   }

         virtual ~Acceptor ()  {   }

         bool listen (typename BaseClass::size_type qsize = 5);
         BaseClass *accept ();
};

// ----------------------------------------------------------------------------

class   RegularSocket;
class   FixedSizeSocket;

typedef Acceptor<RegularSocket>   RegularAcceptor;
typedef Acceptor<FixedSizeSocket> FixedSizeAcceptor;

// ----------------------------------------------------------------------------

#  ifdef DMS_INCLUDE_SOURCE
#    include <Acceptor.tcc>
#  endif // DMS_INCLUDE_SOURCE

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
