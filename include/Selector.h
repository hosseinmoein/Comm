// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <sys/select.h>

#include <vector>

#include <DMScu_FixedSizeString.h>

#include <SocketBase.h>
#include <Pipe.h>

// ----------------------------------------------------------------------------

namespace hmcom
{

class   Selector  {

    public:

        enum OPERATIONS { _read_ = 1, _write_ = 2, _error_ = 4 };
        enum SELECT_RESULT { _read_ready_, _write_ready_, _rw_ready_,
                             _exception_, _timedout_ };

        struct  SelectResult  {

            inline SelectResult () throw ()
                : com (NULL), result (_exception_)  {   }
            inline SelectResult (Communication *c,
                                 SELECT_RESULT r) throw ()
                : com (c), result (r)  {   }

            Communication    *com;
            SELECT_RESULT           result;
        };

        typedef std::vector<SelectResult>   ResultVector;

    private:

        typedef std::vector<Communication *> CommunicationVector;

        CommunicationVector comm_vec_;
        ResultVector	    result_vec_;

    public:

        typedef unsigned int    size_type;

        inline explicit Selector (size_type rev_size) throw ()
            : comm_vec_ (), result_vec_ ()  {   }

        void add_communication (Communication *com) throw ()  {

            if (comm_vec_.empty ())
                comm_vec_.reserve (4);
            comm_vec_.push_back (com);
            return;
        }

        inline void clear () throw ()  { comm_vec_.clear (); }

        bool remove_communication (const Communication &com) throw ()  {

            for (CommunicationVector::iterator citr = comm_vec_.begin ();
                 citr != comm_vec_.end (); ++citr)
                if ((*citr)->get_read_fd () == com.get_read_fd () &&
                    (*citr)->get_write_fd () == com.get_write_fd ())  {
                    comm_vec_.erase (citr);
                    return (true);
                }

            return (false);
        }

        inline const ResultVector &get_result () const throw ()  {

            return (result_vec_);
        }

       // A false return means select timed out. in this case don't bother
       // going through the result vector
       //
        bool select (int operation, long seconds, long mseconds = 0)  {

            result_vec_.clear ();

            fd_set    readfds;
            fd_set    writefds;
            fd_set    errorfds;

            FD_ZERO (&readfds);
            FD_ZERO (&writefds);
            FD_ZERO (&errorfds);

            int max_fd = 0;

            for (CommunicationVector::const_iterator citr = comm_vec_.begin ();
                 citr != comm_vec_.end (); ++citr)  {
                if (operation & _read_) {
                    FD_SET ((*citr)->get_read_fd (), &readfds);
                    if ((*citr)->get_read_fd () > max_fd)
                        max_fd = (*citr)->get_read_fd ();
                    if (operation & _error_)
                        FD_SET ((*citr)->get_read_fd (), &errorfds);
                }

                if (operation & _write_) {
                    FD_SET ((*citr)->get_write_fd (), &writefds);
                    if ((*citr)->get_write_fd () > max_fd)
                        max_fd = (*citr)->get_write_fd ();
                    if (operation & _error_ &&
                        (*citr)->get_write_fd () != (*citr)->get_read_fd ())
                        FD_SET ((*citr)->get_write_fd (), &errorfds);
                }
            }

            timeval tval;

            tval.tv_sec = seconds;
            tval.tv_usec = mseconds;

            const   int rc =
                ::select (max_fd + 1, &readfds, &writefds, &errorfds, &tval);

            if (rc < 0)  {
                DMScu_FixedSizeString<1023> err;

                err.printf ("Selector::select(): ::select(): (%d) %s",
                            errno, strerror (errno));
                throw std::runtime_error(err.c_str ());
            }

            if (rc == 0)
                return (false); // Timed out

            result_vec_.reserve (4);

            for (CommunicationVector::const_iterator citr = comm_vec_.begin ();
                 citr != comm_vec_.end (); ++citr)
                if (FD_ISSET ((*citr)->get_read_fd (), &errorfds) ||
                    FD_ISSET ((*citr)->get_write_fd (), &errorfds))
                    result_vec_.push_back (SelectResult (*citr, _exception_));
                else if (FD_ISSET ((*citr)->get_read_fd (), &writefds) &&
                         FD_ISSET ((*citr)->get_write_fd (), &readfds))
                    result_vec_.push_back (SelectResult (*citr, _rw_ready_));
                else if (FD_ISSET ((*citr)->get_write_fd (), &writefds))
                    result_vec_.push_back (SelectResult(*citr, _write_ready_));
                else if (FD_ISSET ((*citr)->get_read_fd (), &readfds))
                    result_vec_.push_back (SelectResult (*citr, _read_ready_));

            return (true);
        }
};

} // namespace hmcom

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
