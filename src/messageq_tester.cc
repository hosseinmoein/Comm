// Hossein Moein
// March 25, 2018
// Copyright (C) 2018-2019 Hossein Moein
// Distributed under the BSD Software License (see file License)

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <strings.h>
#include <pthread.h>

#include <MessageQueue.h>

// ----------------------------------------------------------------------------

extern "C" void *pusher (void *);
extern "C" void *popper (void *);

struct  test_data  {

    char                name [64];
    int                 i1;
    double              d1;
    char                name2 [4];
    long    long    int lli1;
};

const   unsigned    int Q_SIZE = 10;

// ----------------------------------------------------------------------------

int main (int argCnt, char *argVctr [])  {

    pthread_t   rt;
    pthread_t   wt;
    int         ret;
    bool        writer = true;
    bool        reader = true;

    std::cout.precision (64);

    if (argCnt > 1)  {
        if (! strcasecmp (argVctr [1], "reader"))  {
            writer = false;
        }
        else if (! strcasecmp (argVctr [1], "writer"))  {
            reader = false;
        }
        else  {
            std::cout << "Usage: " << argVctr [0]
                      << " [reader | writer]" << std::endl;
            return (EXIT_FAILURE);
        }
    }

    if (writer)  {
        if ((ret = pthread_create (&wt, NULL, &pusher, NULL)) < 0)  {
            std::cerr << "The thread creation of pusher faild: "
                      << ret << std::endl;
            return (EXIT_FAILURE);
        }
    }

    if (reader)  {
        if ((ret = pthread_create (&rt, NULL, &popper, NULL)) < 0)  {
            std::cerr << "The thread creation of popper faild: "
                      << ret << std::endl;
            return (EXIT_FAILURE);
        }
    }

    void    *dummy;

    if (writer)
        pthread_join (wt, &dummy);
    if (reader)
        pthread_join (rt, &dummy);

    return (EXIT_SUCCESS);
}

// ----------------------------------------------------------------------------

extern "C" void *pusher (void *)  {

    MessageQueue<test_data>  mq("/hossein_test",
                                       MessageQueue<test_data>::_write_,
                                       Q_SIZE);

    struct ::timespec   rqt;

    rqt.tv_sec = 1;
    rqt.tv_nsec = 0;

    char                buffer [1024];
    long    long    int index = 0;
    test_data           data;
    unsigned    int     seed = 1U;

    mq.connect ();
    mq.make_nonblocking ();
    while (true)  {
        if (! (index % 10000))  {
            std::cout << "Current number of messages in queue: "
                      << mq.num_of_msgs_inq () << std::endl;
            nanosleep (&rqt, NULL);
        }

        data.i1 = rand_r (&seed);
        data.d1 = rand_r (&seed);
        data.lli1 = rand_r (&seed);
        sprintf (buffer, "This is the string %lld", index);
        strcpy (data.name, buffer);
        strcpy (data.name2, "!!");
        if ((mq << data) == MessageQueue<test_data>::_would_block_)  {
            std::cout << "message queue is full on " << index << std::endl;
            break;
        }
        index += 1;
    }

    ::strcpy (data.name, "shutdown");
    mq.make_blocking ();
    mq << data;

    mq.disconnect ();
    mq.remove ();
    return (NULL);
}

// ----------------------------------------------------------------------------

extern "C" void *popper (void *)  {

    struct ::timespec   rqt;

    rqt.tv_sec = 1;
    rqt.tv_nsec = 0;

    try  {
        MessageQueue<test_data>  mq (
            "/hossein_test",
            MessageQueue<test_data>::_read_,
            Q_SIZE);
        test_data                       data;

        mq.connect ();
        mq.make_nonblocking ();
        while (true)  {
            if ((mq >> data) ==
                    MessageQueue<test_data>::_would_block_)  {
                std::cout << "message queue is empty\n";
                nanosleep (&rqt, NULL);
                continue;
            }

            std::cout << data.i1 << "\n"
                      << data.d1 << "\n"
                      << data.lli1 << "\n"
                      << data.name << "\n"
                      << data.name2 << "\n\n\n";

            if (! ::strcmp (data.name, "shutdown"))
                break;
        }

        mq.disconnect ();
        // mq.remove (); // It should be already removed above
    }
    catch (const std::exception &ex)  {
        std::cout << "Exception: " << ex.what () << std::endl;
    }

    return (NULL);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
