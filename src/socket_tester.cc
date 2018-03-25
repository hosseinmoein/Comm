// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <iostream>
#include <time.h>
#include <cerrno>
#include <thread>

#include <DMScu_FixedSizeString.h>

#include <FixedSizeSocket.h>
#include <RegularSocket.h>
#include <Acceptor.h>
#include <Pipe.h>
#include <Selector.h>

using namespace hmcom;

// ----------------------------------------------------------------------------

class   Server  {

    public:

        Server (FixedSizeAcceptor &soc) : socket_ (soc)  {   }

        bool the_routine ()  {

            FixedSizeSocket   *soc_ptr;

            try  {
                socket_.listen ();
                soc_ptr = socket_.accept ();

                FixedSizeSocket::HostNameStr peer_name;
                in_port_t                           peer_port;

                soc_ptr->get_peer_host (peer_name, peer_port);
                std::cout << "Server: Peer Host Name: " << peer_name
                          << " -- Peer Port #: " << peer_port << std::endl;

                const    FixedSizeSocket::SELECT_RESULT   sr =
                    soc_ptr->select (FixedSizeSocket::_write_, 300);

                if (sr != FixedSizeSocket::_write_ready_)  {
                    std::cout << "Server::the_routine(): "
                              << "select() returned with unexpected value '"
                              << sr << "'."
                              << std::endl;
                    delete soc_ptr;
                    return (false);
                }
                else  {
                    std::cout << "Server is write ready." << std::endl;
                }

                struct   timespec       rqt;
                const    unsigned   int max_count = 10;

                for (unsigned int i = 0; i < max_count; ++i)  {
                    rqt.tv_sec = 5;
                    rqt.tv_nsec = 0;
                    nanosleep (&rqt, NULL);

                    DMScu_FixedSizeString<1023>  buffer;

                    buffer.printf ("Server data from iteration '%d'", i);
                    soc_ptr->write (buffer.c_str (), buffer.capacity ());
                    std::cout << "Server wrote: '" << buffer.c_str () << "'."
                              << std::endl;
                }
            }
            catch (const std::exception &ex)  {
                std::cout << "Server::the_routine(): Exception\n"
                          << ex.what () << std::endl;
                delete soc_ptr;
                return (false);
            }

            delete soc_ptr;
            return (true);
        }

        FixedSizeAcceptor    &socket_;
};

// ----------------------------------------------------------------------------

class   Client  {

    public:

        Client (FixedSizeSocket &soc) : socket_ (soc)  {   }

        bool the_routine ()  {

            try  {
                struct   timespec       rqt;
                const    unsigned   int max_count = 10;

                const    FixedSizeSocket::SELECT_RESULT   sr =
                    socket_.select (FixedSizeSocket::_read_, 300);

                if (sr != FixedSizeSocket::_read_ready_)  {
                    std::cout << "Client::the_routine(): "
                              << "select() returned with unexpected value '"
                              << sr << "'."
                              << std::endl;
                    return (false);
                }
                else  {
                    std::cout << "Client is read ready." << std::endl;
                }

                FixedSizeSocket::HostNameStr peer_name;
                in_port_t                           peer_port;

                socket_.get_peer_host (peer_name, peer_port);
                std::cout << "Client: Peer Host Name: " << peer_name
                          << " -- Peer Port #: " << peer_port << std::endl;

                for (unsigned int i = 0; i < max_count; ++i)  {
                    rqt.tv_sec = 5;
                    rqt.tv_nsec = 0;
                    nanosleep (&rqt, NULL);

                    FixedSizeSocket::ReadBufferType      *buffer = NULL;
                    const    FixedSizeSocket::size_type  the_size =
                        socket_.read (&buffer);

                    if (the_size > 0)  {
                        buffer [the_size - 1] = 0;
                        std::cout << "Client read: '" << buffer << "'."
                                  << std::endl;
                    }
                    delete[] buffer;
                }
            }
            catch (const std::exception &ex)  {
                std::cout << "Client::the_routine(): Exception\n"
                          << ex.what () << std::endl;
                return (false);
            }

            return (true);
        }

        FixedSizeSocket  &socket_;
};

// ----------------------------------------------------------------------------

class   PipeServer  {

    public:

        PipeServer (Pipe &pipe) : pipe_ (pipe)  {   }

        bool the_routine ()  {

            try  {
                const   Pipe::SELECT_RESULT  sr =
                    pipe_.select (false, 300);

                if (sr != Pipe::_ready_)  {
                    std::cout << "PipeServer::the_routine(): "
                              << "select() returned with unexpected value '"
                              << sr << "'."
                              << std::endl;
                    return (false);
                }
                else  {
                    std::cout << "PipeServer is write ready." << std::endl;
                }

                struct   timespec       rqt;
                const    unsigned   int max_count = 10;

                for (unsigned int i = 0; i < max_count; ++i)  {
                    rqt.tv_sec = 5;
                    rqt.tv_nsec = 0;
                    nanosleep (&rqt, NULL);

                    DMScu_FixedSizeString<31>  buffer;

                    buffer.printf ("%02d", i);
                    pipe_.send (buffer.c_str (), 2);
                    std::cout << "PipeServer wrote: '" << buffer.c_str ()
                              << "'." << std::endl;
                }
            }
            catch (const std::exception &ex)  {
                std::cout << "PipeServer::the_routine(): Exception\n"
                          << ex.what () << std::endl;
                return (false);
            }

            return (true);
        }

        Pipe &pipe_;
};

// ----------------------------------------------------------------------------

class   PipeClient  {

    public:

        PipeClient (Pipe &pipe) : pipe_ (pipe)  {   }

        bool the_routine ()  {

            try  {
                struct  timespec                      rqt;
                const   unsigned    int               max_count = 10;
                const    Pipe::SELECT_RESULT   sr =
                    pipe_.select (true, 300);

                if (sr != Pipe::_ready_)  {
                    std::cout << "PipeClient::the_routine(): "
                              << "select() returned with unexpected value '"
                              << sr << "'." << std::endl;
                    return (false);
                }
                else  {
                    std::cout << "PipeClient is read ready." << std::endl;
                }

                for (unsigned int i = 0; i < max_count; ++i)  {
                    rqt.tv_sec = 5;
                    rqt.tv_nsec = 0;
                    nanosleep (&rqt, NULL);

                    char                                        buffer [3];
                    const   FixedSizeSocket::size_type   the_size =
                        pipe_.receive (&buffer, 2);

                    if (the_size > 0)  {
                        buffer [the_size] = 0;
                        std::cout << "PipeClient read: '" << buffer << "'."
                                  << std::endl;
                    }
                }
            }
            catch (const std::exception &ex)  {
                std::cout << "PipeClient::the_routine(): Exception\n"
                          << ex.what () << std::endl;
                return (false);
            }

            return (true);
        }

        Pipe &pipe_;
};

// ----------------------------------------------------------------------------

static  const   char        *HOST_NAME = "hossein-VirtualBox";
static  const   in_port_t   PORT_NUM = 53581;

int main (int argCnt, char *argVctr [])  {

    FixedSizeAcceptor    ssoc ("server_test",
                                      PORT_NUM,
                                      FixedSizeSocket::_name_,
                                      HOST_NAME);
    FixedSizeSocket      csoc ("client_test",
                                      FixedSizeSocket::_ipv4_,
                                      FixedSizeSocket::_stream_,
                                      FixedSizeSocket::_client_,
                                      PORT_NUM,
                                      FixedSizeSocket::_name_,
                                      HOST_NAME);
    Pipe                 pipe;

    pipe.connect ();

    std::cout << "main(): Connecting the Server." << std::endl;
    ssoc.connect ();

    Server         server (ssoc);
    std::thread    sthr (&Server::the_routine, &server);

    struct   timespec    rqt;

    std::cout << "Going on sleep to lauch Client ..." << std::endl;
    rqt.tv_sec = 10;
    rqt.tv_nsec = 0;
    nanosleep (&rqt, NULL);
    std::cout << "... Launching Client now" << std::endl;

    std::cout << "main(): Connecting the Client." << std::endl;
    csoc.connect ();

    Client         client (csoc);
    std::thread    cthr (&Client::the_routine, &client);

    std::cout << "main(): Connecting the PipeServer." << std::endl;

    PipeServer     pipe_server (pipe);
    std::thread    psthr (&PipeServer::the_routine, &pipe_server);

    std::cout << "Going on sleep to lauch PipeClient ..." << std::endl;
    rqt.tv_sec = 10;
    rqt.tv_nsec = 0;
    nanosleep (&rqt, NULL);
    std::cout << "... Launching PipeClient now" << std::endl;

    PipeClient     pipe_client (pipe);
    std::thread    pcthr (&PipeClient::the_routine, &pipe_client);

    std::cout << "\n\tTesting the Selector ...\n" << std::endl;

    FixedSizeSocket  soc1 ("socket1_test",
                                  FixedSizeSocket::_ipv4_,
                                  FixedSizeSocket::_stream_,
                                  FixedSizeSocket::_client_,
                                  PORT_NUM,
                                  FixedSizeSocket::_name_,
                                  HOST_NAME);
    FixedSizeSocket  soc2 ("socket2_test",
                                  FixedSizeSocket::_ipv4_,
                                  FixedSizeSocket::_stream_,
                                  FixedSizeSocket::_client_,
                                  PORT_NUM,
                                  FixedSizeSocket::_name_,
                                  HOST_NAME);

    soc1.connect ();
    soc2.connect ();

    Selector                 selector (4);

    selector.add_communication (&ssoc);
    selector.add_communication (&csoc);
    selector.add_communication (&soc1);
    selector.add_communication (&soc2);

    selector.add_communication (&pipe);

    selector.select (Selector::_read_ | Selector::_error_, 30);

    const   Selector::ResultVector   &res1 = selector.get_result ();

    for (Selector::ResultVector::const_iterator citr = res1.begin ();
         citr != res1.end (); ++citr)  {
        std::cout << "[" << citr->com->get_read_fd () << ","
                  << citr->com->get_write_fd () << "] -- "
                  << citr->result << std::endl;
    }

    std::cout << std::endl;

    selector.select (Selector::_write_ | Selector::_error_, 30);

    const   Selector::ResultVector   &res2 = selector.get_result ();

    for (Selector::ResultVector::const_iterator citr = res2.begin ();
         citr != res2.end (); ++citr)  {
        std::cout << "[" << citr->com->get_read_fd () << ","
                  << citr->com->get_write_fd () << "] -- "
                  << citr->result << std::endl;
    }

    std::cout << "main(): Waiting for the threads." << std::endl;
    sthr.join ();
    cthr.join ();
    psthr.join ();
    pcthr.join ();

    std::cout << "main(): Exiting." << std::endl;

    ssoc.disconnect ();
    csoc.disconnect ();

    std::cout << "\n\tTesting the get_hostname_by_ip() ...\n" << std::endl;

    DMScu_FixedSizeString<63>  hostname;
    DMScu_FixedSizeString<63>  server_name;

    SocketBase::get_hostname_by_ip ("10.94.7.38", hostname, server_name);

    std::cout << "Node is " << hostname << std::endl
              << "Server is " << server_name << std::endl;

    return (EXIT_SUCCESS);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
