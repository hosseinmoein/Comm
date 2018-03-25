// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#pragma once

#include <cstdlib>
#include <string>

// ----------------------------------------------------------------------------

class   Communication  {

    public:

        typedef unsigned int    size_type;

        enum TYPE { _undefined_, _socket_, _pipe_, _message_q_ };
        enum ERROR_CODE { _not_implemented_ = -1, _try_again_ = -2 };

        inline Communication (const char *name) throw ()
            : name_ (name), connected_ (false), blocking_ (true)  {   }
        inline virtual ~Communication ()  {   }

        inline bool connect ()  {

            if (_connect_hook ())  {
                connected_ = true;
                return (true);
            }
            else
                return (false);
        }
        inline bool disconnect ()  {

            if (_disconnect_hook ())  {
                connected_ = false;
                return (true);
            }
            else
                return (false);
        }

        virtual int send (const void *data, size_type the_size)  {

            return (_not_implemented_);
        }
        virtual int receive (void *data, size_type the_size)  {

            return (_not_implemented_);
        }

        inline bool make_blocking ()  {

            if (_make_blocking_hook ())  {
                blocking_ = true;
                return (true);
            }
            else
                return (false);
        }
        inline bool make_nonblocking ()  {

            if (_make_nonblocking_hook ())  {
                blocking_ = false;
                return (true);
            }
            else
                return (false);
        }

        inline const char *get_name () const throw ()  {

            return (name_.c_str ());
        }
        inline bool is_connected () const throw ()  { return (connected_); }
        inline bool is_blocking () const throw ()  { return (blocking_); }
        inline void set_connected (bool value) throw ()  {

            connected_ = value;
        }

        virtual int get_fd () const throw ()  { return (-1); }
        virtual int get_read_fd () const throw ()  { return (get_fd ()); }
        virtual int get_write_fd () const throw ()  { return (get_fd ()); }

        virtual TYPE get_type () const throw ()  { return (_undefined_); }

    protected:

        virtual bool _connect_hook () = 0;
        virtual bool _disconnect_hook () = 0;

        virtual bool _make_blocking_hook ()  { return (false); }
        virtual bool _make_nonblocking_hook ()  { return (false); }

        inline void _set_blocking (bool value)  { blocking_ = value; }

    private:

        std::string name_;
        bool        connected_;
        bool        blocking_;
};

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:

