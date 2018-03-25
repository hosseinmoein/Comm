// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <netinet/in.h>

#include <DMScu_FixedSizeString.h>

#include <Communication.h>

// ----------------------------------------------------------------------------

struct  sockaddr_in;
struct  ip_mreqn;

// ----------------------------------------------------------------------------

class   SocketBase : public Communication  {

    public:

        typedef Communication        BaseClass;
        typedef DMScu_FixedSizeString<63>   HostNameStr;

        enum IP_ADDRESS_TYPE { _ipv4_, _ipv6_ };
        enum SOCKET_TYPE { _stream_, _dgram_ };
        enum SOCKET_RULE { _server_, _client_ };
        enum HOSTNAME_TYPE { _name_, _ip_address_ };
        enum ORIENTATION { _connected_, _connectionless_ };
        enum OPERATIONS { _read_ = 1, _write_ = 2, _error_ = 4 };
        enum SELECT_RESULT { _read_ready_, _write_ready_, _rw_ready_,
                             _exception_, _timedout_ };

        inline SocketBase (const char *name,
                                  IP_ADDRESS_TYPE ip_address_type,
                                  SOCKET_TYPE socket_type,
                                  SOCKET_RULE socket_rule,
                                  in_port_t port,
                                  HOSTNAME_TYPE hostname_type = _name_,
                                  const char *hostname = NULL,
                                  ORIENTATION orientation = _connected_)
            throw ()
            : BaseClass (name),
              fd_ (0),
              ip_address_type_ (ip_address_type),
              socket_type_ (socket_type),
              socket_rule_ (socket_rule),
              port_ (port),
              hostname_type_ (hostname_type),
              hostname_ (hostname ? hostname : ""),
              orientation_ (orientation)  {   }

        inline virtual ~SocketBase ()  {

            if (is_connected ())
                disconnect ();
        }

        bool set_send_buffer_size (int buf_size);
        bool set_receive_buffer_size (int buf_size);

        int get_free_send_buffer_space ();

        bool disable_nagel_algorithm ();

        inline IP_ADDRESS_TYPE get_ip_address_type () const throw ()  {

            return (ip_address_type_);
        }
        inline SOCKET_TYPE get_socket_type () const throw ()  {

            return (socket_type_);
        }
        inline in_port_t get_port () const throw ()  { return (port_); }
        inline HOSTNAME_TYPE get_hostname_type () const throw ()  {

            return (hostname_type_);
        }
        inline const char *get_hostname () const throw ()  {

            return (hostname_.c_str ());
        }

       // operation must be a bitwise value of OPERATIONS
       //
        SELECT_RESULT select (int operation, long seconds, long mseconds = 0);

       // A back door. don't abuse it
       //
        virtual int get_fd () const throw ()  { return (fd_); }
        inline void set_fd (int fd) throw ()  { fd_ = fd; }

        virtual TYPE get_type () const throw ()  { return (_socket_); }

        ORIENTATION get_orientation () const throw ()  {

            return (orientation_);
        }

        inline bool get_peer_host (DMScu_VirtualString &hostname,
                                   in_port_t &port) const  {

            return (get_peername_by_fd (get_fd (),
                                        hostname,
                                        port,
                                        get_ip_address_type ()));
        }

    protected:

        virtual bool _connect_hook ();
        virtual bool _disconnect_hook ();

        virtual bool _make_blocking_hook ();
        virtual bool _make_nonblocking_hook ();

        bool _get_sock_addr_in (struct sockaddr_in &sai_addr,
                                struct ip_mreqn &ipm_addr) const;

    private:

        int                     fd_;
        const   IP_ADDRESS_TYPE ip_address_type_;
        const   SOCKET_TYPE     socket_type_;
        const   SOCKET_RULE     socket_rule_;
        const   in_port_t       port_;
        const   HOSTNAME_TYPE   hostname_type_;
        const   HostNameStr     hostname_;
        const   ORIENTATION     orientation_;

    public:

        static bool get_peername_by_fd (int fd,
                                        DMScu_VirtualString &host_name,
                                        in_port_t &port,
                                        IP_ADDRESS_TYPE ip_at = _ipv4_);

        static bool get_hostname_by_ip (const char *dotnotation_ip,
                                        DMScu_VirtualString &host_name,
                                        DMScu_VirtualString &server_name,
                                        IP_ADDRESS_TYPE ip_at = _ipv4_);
};

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
