// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <iostream>
#include <cerrno>
#include <stdexcept>
#include <sstream>

#include <DMScu_FixedSizeString.h>

#include <MessageQueue.h>

// ----------------------------------------------------------------------------

namespace hmcom
{

template <class com_TYPE>
bool MessageQueue<com_TYPE>::_connect_hook ()  {

    if (! is_connected ())  {
        int oflag = O_CREAT;

        if (open_mode_ == _read_)
            oflag |= O_RDONLY;
        else if (open_mode_ == _write_)
            oflag |= O_WRONLY;
        else if (open_mode_ == _read_write_)
            oflag |= O_RDWR;
        else  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::_connect_hook(): "
                        "open mode (%d) is not valid.",
                        open_mode_);
            throw std::runtime_error(err.c_str ());
        }

        struct  mq_attr attr;

        ::memset (&attr, 0, sizeof (struct  mq_attr));
        // attr.mq_flags = 0;
        // attr.mq_comrmsgs = 0;
        attr.mq_maxmsg = max_msg_num_;
        attr.mq_msgsize = msg_size_;

        mqdes_ = ::mq_open (get_name (), oflag, permission_, &attr);

        if (mqdes_ == static_cast<mqd_t>(-1))  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::_connect_hook(): "
                        "::mq_open() (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

        return (true);
    }

    return (false);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
bool MessageQueue<com_TYPE>::_disconnect_hook ()  {

    if (is_connected ())  {
        ::mq_close (mqdes_);
        mqdes_ = static_cast<mqd_t>(-1);
        return (true);
    }

    return (false);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
bool MessageQueue<com_TYPE>::_make_nonblocking_hook ()  {

    if (is_blocking ())  {
        const   struct  mq_attr attr = { O_NONBLOCK, 0, 0, 0 };

        // attr.mq_flags = O_NONBLOCK;
        // attr.mq_curmsgs = 0;
        // attr.mq_maxmsg = 0;
        // attr.mq_msgsize = 0;

        if (::mq_setattr (mqdes_, &attr, NULL) < 0)  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::_make_nonblocking_hook(): "
                        "::mq_setattr() (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }

    return (true);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
bool MessageQueue<com_TYPE>::_make_blocking_hook ()  {

    if (! is_blocking ())  {
        const   struct  mq_attr attr = { 0, 0, 0, 0 };

        // attr.mq_flags = 0;
        // attr.mq_curmsgs = 0;
        // attr.mq_maxmsg = 0;
        // attr.mq_msgsize = 0;

        if (::mq_setattr (mqdes_, &attr, NULL) < 0)  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::_make_blocking_hook(): "
                        "::mq_setattr() (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }
    }

    return (true);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
typename MessageQueue<com_TYPE>::size_type
MessageQueue<com_TYPE>::num_of_msgs_inq () const  {

    struct  mq_attr attr;

    if (::mq_getattr (mqdes_, &attr) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("MessageQueue::num_of_msgs_inq(): "
                    "::mq_setattr() (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return (attr.mq_curmsgs);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
typename MessageQueue<com_TYPE>::size_type
MessageQueue<com_TYPE>::push (const value_type &data,
                                     priority_type priority)  {

    if (open_mode_ == _read_)
        throw std::runtime_error ("DMScu_MessageQueue::push(): "
                                "message queue is read only.");

    const   int ret_val =
        ::mq_send (mqdes_,
                   reinterpret_cast<const char *> (&data),
                   msg_size_,
                   priority);

    if (ret_val < 0)
        if (errno == EAGAIN)
            return (static_cast<size_type>(_would_block_));
        else  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::push(): "
                        "::mq_send() (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

    return (ret_val);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
typename MessageQueue<com_TYPE>::size_type
MessageQueue<com_TYPE>::pop (value_type &data,
                                    priority_type *priority)  {

    if (open_mode_ == _write_)
        throw std::runtime_error ("MessageQueue::pop(): "
                                "message queue is write only.");

    const   int ret_val =
        ::mq_receive (mqdes_,
                      reinterpret_cast<char *> (&data),
                      msg_size_,
                      priority);

    if (ret_val < 0)
        if (errno == EAGAIN)
            return (static_cast<size_type>(_would_block_));
        else  {
            DMScu_FixedSizeString<1023> err;

            err.printf ("MessageQueue::pop(): "
                        "::mq_receive() (%d) %s",
                        errno, strerror (errno));
            throw std::runtime_error(err.c_str ());
        }

    return (ret_val);
}

// ----------------------------------------------------------------------------

template <class com_TYPE>
void MessageQueue<com_TYPE>::remove ()  {

    disconnect ();
    if (::mq_unlink (get_name ()) < 0)  {
        DMScu_FixedSizeString<1023> err;

        err.printf ("MessageQueue::remove(): "
                    "::mq_unlink() (%d) %s",
                    errno, strerror (errno));
        throw std::runtime_error(err.c_str ());
    }

    return;
}

} // namespace hmcom

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
