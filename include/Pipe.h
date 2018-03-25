// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <unistd.h>
#include <stdexcept>

#include <Communication.h>

// ----------------------------------------------------------------------------

namespace hmcom
{

class   Pipe : public Communication  {

    // Note: this class has basic mutual exclusion, but it's the user's
    // responsibility not to close a pipe that someone else is still
    // using.

    public:

        typedef Communication    BaseClass;

        enum SELECT_RESULT { _ready_, _exception_, _timedout_ };

        inline explicit Pipe (const char *name = "") throw ()
		    : BaseClass (name),
              filedes_ ()  {

            filedes_ [0] = -1;
            filedes_ [1] = -1;
        }

        inline ~Pipe ()  { disconnect (); }

    protected:

        inline bool _connect_hook ()  {

            if (is_connected ())
                return (false);

            const   int result = ::pipe (filedes_);

            if (result == -1)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::_connect_hook(): ::pipe(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            return (true);
        }

        inline bool _disconnect_hook ()  {

            if (! is_connected ())
                return (false);

            int result = ::close (filedes_[0]);

            if (result == -1)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::close(): ::close(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            result = ::close (filedes_[1]);
            if (result == -1)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::close(): ::close(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            filedes_ [0] = -1;
            filedes_ [1] = -1;
            return (true);
        }

    public:

        SELECT_RESULT
        select (bool read, size_type seconds, size_type mseconds = 0)  {

            fd_set  readfds;
            fd_set  writefds;
            fd_set  errorfds;

            FD_ZERO (&readfds);
            FD_ZERO (&writefds);
            FD_ZERO (&errorfds);

            if (read)  {
                FD_SET (get_read_fd (), &readfds);
                FD_SET (get_read_fd (), &errorfds);
            }
            else  {
                FD_SET (get_write_fd (), &writefds);
                FD_SET (get_write_fd (), &errorfds);
            }

            timeval tval;

            tval.tv_sec = seconds;
            tval.tv_usec = mseconds;

            const   int rc =
                ::select ((read ? get_read_fd () : get_write_fd ()) + 1,
                          &readfds,
                          &writefds,
                          &errorfds,
                          &tval);

            if (rc < 0)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::select(): ::select(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            if (rc == 0)
                return (_timedout_);
            if (read && FD_ISSET (get_read_fd (), &errorfds))
                return (_exception_);
            if (!read && FD_ISSET (get_write_fd (), &errorfds))
                return (_exception_);
            if (read && FD_ISSET (get_read_fd (), &readfds))
                return (_ready_);
            if (!read && FD_ISSET (get_write_fd (), &writefds))
                return (_ready_);
        }

        virtual int receive (void *data, size_type the_size)  {

            const   int received_size = ::read (get_read_fd(), data, the_size);

            if (received_size < 0)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::read(): ::read(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            return (received_size);
        }

        virtual int send (const void *data, size_type the_size)  {

            const   int sent_size = ::write (get_write_fd (), data, the_size);

            if (sent_size < 0)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Pipe::write(): ::write(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            return (sent_size);
        }

        virtual int get_read_fd () const throw ()  { return (filedes_ [0]); }
        virtual int get_write_fd () const throw ()  { return (filedes_ [1]); }

        virtual TYPE get_type () const throw ()  { return (_pipe_); }

    private:

        int filedes_ [2];

      // These are not implemented
      //
        Pipe (const Pipe &);
        Pipe &operator = (const Pipe &);
};

} // namespace hmcom

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
