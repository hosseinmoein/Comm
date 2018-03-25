// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <cstdlib>
#include <unistd.h>
#include <string>
#include <stdexcept>

#include <mqueue.h>

#include <Communication.h>

// ----------------------------------------------------------------------------

namespace hmcom
{

// This class encapsulates POSIX message queues. There are limitations on
// com_TYPE. com_TYPE elements are memcpy()'ed in and out of POSIX
// message queues. Therefore a com_TYPE cannot have any virtual method or any
// dynamically allocated member or anything that will break as a result of
// memcpy().
//
template <class com_TYPE>
class   MessageQueue : public Communication  {

    public:

        typedef Communication            BaseClass;
        typedef com_TYPE                        value_type;
        typedef typename BaseClass::size_type   size_type;
        typedef unsigned int                    priority_type;

        enum MODE { _read_, _write_, _read_write_ };
        enum RET_TYPE { _would_block_ = -2 };

        MessageQueue (const char *name,
                             MODE open_mode,
                             size_type max_msg_num = 10000UL,
                             mode_t permission = 0740,
                             size_type msg_size = sizeof (value_type)) throw ()
            : BaseClass (name),
              mqdes_ (static_cast<mqd_t>(-1)),
              msg_size_ (msg_size),
              max_msg_num_ (max_msg_num),
              permission_ (permission),
              open_mode_ (open_mode)  {   }

        virtual ~MessageQueue ()  { disconnect (); }

        size_type num_of_msgs_inq () const;

        size_type push (const value_type &data, priority_type priority = 1);
        size_type pop (value_type &data, priority_type *priority = NULL);

        inline size_type operator >> (value_type &data)  {

            return (pop (data));
        }
        inline size_type operator << (const value_type &data)  {

            return (push (data));
        }

        void remove ();

        virtual int get_fd () const throw ()  {

            return (static_cast<int>(mqdes_));
        }
        virtual TYPE get_type () const throw ()  { return (_message_q_); }

    protected:

        virtual bool _make_blocking_hook ();
        virtual bool _make_nonblocking_hook ();
        virtual bool _connect_hook ();
        virtual bool _disconnect_hook ();

    private:

        mqd_t               mqdes_;
        const   size_type   msg_size_;
        const   size_type   max_msg_num_;
        const   mode_t      permission_;
        const   MODE        open_mode_;

       // These are not implemented
       //
        MessageQueue ();
        MessageQueue (const MessageQueue &);
        MessageQueue &operator = (const MessageQueue &);
};

} // namespace hmcom

// ----------------------------------------------------------------------------

#  ifdef DMS_INCLUDE_SOURCE
#    include <MessageQueue.tcc>
#  endif // DMS_INCLUDE_SOURCE

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
