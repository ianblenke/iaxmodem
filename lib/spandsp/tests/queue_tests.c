/*
 * SpanDSP - a series of DSP components for telephony
 *
 * queue_tests.c
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2007 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: queue_tests.c,v 1.2 2007/05/02 14:26:29 steveu Exp $
 */

/* THIS IS A WORK IN PROGRESS. IT IS NOT FINISHED. */

/*! \page queue_tests_page Queue tests
\section queue_tests_page_sec_1 What does it do?
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <assert.h>
#include <tiffio.h>
#include <pthread.h>
#include <sched.h>

#include "spandsp.h"

#define BUF_LEN     10000
#define MSG_LEN     17

pthread_t thread[2];
queue_state_t *queue;
volatile int put_oks;
volatile int put_misses;
volatile int got_oks;
volatile int got_misses;

int total_in;
int total_out;

static void *run_stream_write(void *arg)
{
    uint8_t buf[MSG_LEN];
    int i;
    int next;

    printf("Write thread\n");
    next = 0;
    for (i = 0;  i < MSG_LEN;  i++)
        buf[i] = next;
    next = (next + 1) & 0xFF;
    put_oks = 0;
    put_misses = 0;
    for (;;)
    {
        if (queue_write(queue, buf, MSG_LEN) == MSG_LEN)
        {
            for (i = 0;  i < MSG_LEN;  i++)
                buf[i] = next;
            next = (next + 1) & 0xFF;
            put_oks++;
            if (put_oks%1000000 == 0)
                printf("%d puts, %d misses\n", put_oks, put_misses);
        }
        else
        {
            sched_yield();
            put_misses++;
        }
    }
    return NULL;
}
/*- End of function --------------------------------------------------------*/

static void *run_stream_read(void *arg)
{
    uint8_t buf[MSG_LEN];
    int i;
    int len;
    int next;

    printf("Read thread\n");
    next = 0;
    got_oks = 0;
    got_misses = 0;
    for (;;)
    {
        if ((len = queue_read(queue, buf, MSG_LEN)) >= 0)
        {
            if (len != MSG_LEN)
            {
                printf("AHH! - len %d\n", len);
                exit(2);
            }
            for (i = 0;  i < len;  i++)
            {
                if (buf[i] != next)
                {
                    printf("AHH! - 0x%X 0x%X\n", buf[i], next);
                    exit(2);
                }
            }
            next = (next + 1) & 0xFF;
            got_oks++;
            if (got_oks%1000000 == 0)
                printf("%d gots, %d misses\n", got_oks, got_misses);
        }
        else
        {
            sched_yield();
            got_misses++;
        }
    }
    return NULL;
}
/*- End of function --------------------------------------------------------*/

static void threaded_stream_tests(void)
{
    pthread_attr_t attr;

    if ((queue = queue_create(BUF_LEN, QUEUE_READ_ATOMIC | QUEUE_WRITE_ATOMIC)) == NULL)
    {
        printf("Failed to create the queue\n");
        exit(2);
    }
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread[0], &attr, run_stream_write, NULL))
    {
        printf("Failed to create thread\n");
        exit(2);
    }
    if (pthread_create(&thread[1], &attr, run_stream_read, NULL))
    {
        printf("Failed to create thread\n");
        exit(2);
    }
    for (;;)
    {
        sleep(5);
        printf("Main thread - %d %d\n", put_oks, got_oks);
    }
    queue_delete(queue);
}
/*- End of function --------------------------------------------------------*/

static void *run_message_write(void *arg)
{
    uint8_t buf[MSG_LEN];
    int i;
    int next;

    printf("Write thread\n");
    next = 0;
    for (i = 0;  i < MSG_LEN;  i++)
        buf[i] = next;
    next = (next + 1) & 0xFF;
    put_oks = 0;
    put_misses = 0;
    for (;;)
    {
        if (queue_write_msg(queue, buf, MSG_LEN) == MSG_LEN)
        {
            for (i = 0;  i < MSG_LEN;  i++)
                buf[i] = next;
            next = (next + 1) & 0xFF;
            put_oks++;
            if (put_oks%1000000 == 0)
                printf("%d puts, %d misses\n", put_oks, put_misses);
        }
        else
        {
            sched_yield();
            put_misses++;
        }
    }
    return NULL;
}
/*- End of function --------------------------------------------------------*/

static void *run_message_read(void *arg)
{
    uint8_t buf[1024];
    int i;
    int len;
    int next;

    printf("Read thread\n");
    next = 0;
    got_oks = 0;
    got_misses = 0;
    for (;;)
    {
        if ((len = queue_read_msg(queue, buf, 1024)) >= 0)
        {
            if (len != MSG_LEN)
            {
                printf("AHH! - len %d\n", len);
                exit(2);
            }
            for (i = 0;  i < len;  i++)
            {
                if (buf[i] != next)
                {
                    printf("AHH! - 0x%X 0x%X\n", buf[i], next);
                    exit(2);
                }
            }
            next = (next + 1) & 0xFF;
            got_oks++;
            if (got_oks%1000000 == 0)
                printf("%d gots, %d misses\n", got_oks, got_misses);
        }
        else
        {
            sched_yield();
            got_misses++;
        }
    }
    return NULL;
}
/*- End of function --------------------------------------------------------*/

static void threaded_message_tests(void)
{
    pthread_attr_t attr;

    if ((queue = queue_create(BUF_LEN, QUEUE_READ_ATOMIC | QUEUE_WRITE_ATOMIC)) == NULL)
    {
        printf("Failed to create the queue\n");
        exit(2);
    }
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread[0], &attr, run_message_write, NULL))
    {
        printf("Failed to create thread\n");
        exit(2);
    }
    if (pthread_create(&thread[1], &attr, run_message_read, NULL))
    {
        printf("Failed to create thread\n");
        exit(2);
    }
    for (;;)
    {
        sleep(5);
        printf("Main thread - %d %d\n", put_oks, got_oks);
    }
    queue_delete(queue);
}
/*- End of function --------------------------------------------------------*/

static void check_contents(int total_in, int total_out)
{
    if (queue_contents(queue) != (total_in - total_out))
    {
        printf("Contents = %d (%d)\n", queue_contents(queue), (total_in - total_out));
        printf("Pointers: %d %d %d\n", queue->iptr, queue->optr, queue->len);
        exit(2);
    }
    if (queue_free_space(queue) != BUF_LEN - (total_in - total_out))
    {
        printf("Free space = %d (%d)\n", queue_free_space(queue), BUF_LEN - (total_in - total_out));
        printf("Pointers: %d %d %d\n", queue->iptr, queue->optr, queue->len);
        exit(2);
    }
}
/*- End of function --------------------------------------------------------*/

static int monitored_queue_write(const uint8_t buf[], int len)
{
    int lenx;
    
    lenx = queue_write(queue, buf, len);
    if (lenx >= 0)
        total_in += lenx;
    check_contents(total_in, total_out);
    return lenx;
}
/*- End of function --------------------------------------------------------*/

static int monitored_queue_read(uint8_t buf[], int len)
{
    int lenx;
    
    lenx = queue_read(queue, buf, len);
    if (lenx >= 0)
        total_out += lenx;
    check_contents(total_in, total_out);
    return lenx;
}
/*- End of function --------------------------------------------------------*/

static void functional_stream_tests(void)
{
    uint8_t buf[MSG_LEN];
    int i;
    
    total_in = 0;
    total_out = 0;

    for (i = 0;  i < MSG_LEN;  i++)
        buf[i] = i;
    if ((queue = queue_create(BUF_LEN, QUEUE_READ_ATOMIC | QUEUE_WRITE_ATOMIC)) == NULL)
    {
        printf("Failed to create the queue\n");
        exit(2);
    }
    check_contents(total_in, total_out);
    /* Fill the buffer, checking the contents grow correctly */
    for (i = 1;  i < 1000;  i++)
    {
        if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
            break;
    }
    printf("Full at chunk %d (expected %d)\n", i, BUF_LEN/MSG_LEN + 1);
    if (monitored_queue_write(buf, 5) == 5)
    {
        printf("Write of 5 succeeded\n");
        exit(2);
    }
    if (monitored_queue_write(buf, 4) != 4)
    {
        printf("Write of 4 failed\n");
        exit(2);
    }
    /* Now full. Empty a little, and refill around the end */
    if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
    {
        printf("Read failed\n");
        exit(2);
    }
    if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
    {
        printf("Write failed\n");
        exit(2);
    }
    /* Empty completely, checking the contents shrink correctly */
    for (;;)
    {
        if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
            break;
    }
    if (monitored_queue_read(buf, 4) != 4)
    {
        printf("Read failed\n");
        exit(2);
    }
    /* Nudge around the buffer */
    for (i = 1;  i < 588;  i++)
    {
        if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Write failed\n");
            exit(2);
        }
        if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Read failed\n");
            exit(2);
        }
    }
    /* Fill the buffer, checking the contents grow correctly */
    for (i = 1;  i < 1000;  i++)
    {
        if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
            break;
    }
    printf("Full at chunk %d (expected %d)\n", i, BUF_LEN/MSG_LEN + 1);
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
    {
        printf("Read failed\n");
        exit(2);
    }
    if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
    {
        printf("Write failed\n");
        exit(2);
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    for (i = 1;  i < 5000;  i++)
    {
        if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Read failed\n");
            exit(2);
        }
        if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Write failed\n");
            exit(2);
        }
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    if (monitored_queue_write(buf, 5) == 5)
    {
        printf("Write of 5 succeeded\n");
        exit(2);
    }
    if (monitored_queue_write(buf, 4) != 4)
    {
        printf("Write of 4 failed\n");
        exit(2);
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    for (i = 1;  i < 5000;  i++)
    {
        if (monitored_queue_read(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Read failed\n");
            exit(2);
        }
        if (monitored_queue_write(buf, MSG_LEN) != MSG_LEN)
        {
            printf("Write failed\n");
            exit(2);
        }
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    queue_delete(queue);
}
/*- End of function --------------------------------------------------------*/

static int monitored_queue_write_msg(const uint8_t buf[], int len)
{
    int lenx;
    
    lenx = queue_write_msg(queue, buf, len);
    if (lenx >= 0)
        total_in += lenx + sizeof(uint16_t);
    check_contents(total_in, total_out);
    return lenx;
}
/*- End of function --------------------------------------------------------*/

static int monitored_queue_read_msg(uint8_t buf[], int len)
{
    int lenx;

    lenx = queue_read_msg(queue, buf, len);
    if (lenx >= 0)
        total_out += lenx + sizeof(uint16_t);
    check_contents(total_in, total_out);
    return lenx;
}
/*- End of function --------------------------------------------------------*/

static void functional_message_tests(void)
{
    uint8_t buf[MSG_LEN];
    int i;
    int len;
    
    total_in = 0;
    total_out = 0;

    for (i = 0;  i < MSG_LEN;  i++)
        buf[i] = i;
    if ((queue = queue_create(BUF_LEN, QUEUE_READ_ATOMIC | QUEUE_WRITE_ATOMIC)) == NULL)
    {
        printf("Failed to create the queue\n");
        exit(2);
    }
    check_contents(total_in, total_out);
    /* Fill the buffer, checking the contents grow correctly */
    for (i = 1;  i < 1000;  i++)
    {
        if (monitored_queue_write_msg(buf, MSG_LEN) != MSG_LEN)
            break;
    }
    printf("Full at chunk %d (expected %d)\n", i, BUF_LEN/(MSG_LEN + sizeof(uint16_t)) + 1);
    if ((len = monitored_queue_write_msg(buf, 5)) == 5)
    {
        printf("Write of 5 succeeded\n");
        exit(2);
    }
    if ((len = monitored_queue_write_msg(buf, 4)) != 4)
    {
        printf("Write of 4 failed\n");
        exit(2);
    }
    /* Now full. Empty a little, and refill around the end */
    if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
    {
        printf("Read failed - %d\n", len);
        exit(2);
    }
    if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
    {
        printf("Write failed - %d\n", len);
        exit(2);
    }
    /* Empty completely, checking the contents shrink correctly */
    for (;;)
    {
        if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
            break;
    }
    if (len != 4)
    {
        printf("Read failed - %d\n", len);
        exit(2);
    }
    /* Nudge around the buffer */
    for (i = 1;  i < 527;  i++)
    {
        if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Write failed - %d\n", len);
            exit(2);
        }
        if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Read failed - %d\n", len);
            exit(2);
        }
    }
    /* Fill the buffer, checking the contents grow correctly */
    for (i = 1;  i < 1000;  i++)
    {
        if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
            break;
    }
    printf("Full at chunk %d (expected %d)\n", i, BUF_LEN/(MSG_LEN + sizeof(uint16_t)) + 1);
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);

    if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
    {
        printf("Read failed - %d\n", len);
        exit(2);
    }
    if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
    {
        printf("Write failed - %d\n", len);
        exit(2);
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    for (i = 1;  i < 5000;  i++)
    {
        if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Read failed - %d\n", len);
            exit(2);
        }
        if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Write failed - %d\n", len);
            exit(2);
        }
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    if ((len = monitored_queue_write_msg(buf, 5)) == 5)
    {
        printf("Write of 5 succeeded\n");
        exit(2);
    }
    if ((len = monitored_queue_write_msg(buf, 4)) != 4)
    {
        printf("Write of 4 failed\n");
        exit(2);
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    for (i = 1;  i < 5000;  i++)
    {
        if (i == 527)
        {
            if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != 4)
            {
                printf("Read failed - %d\n", len);
                exit(2);
            }
        }
        if ((len = monitored_queue_read_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Read failed - %d\n", len);
            exit(2);
        }
        if ((len = monitored_queue_write_msg(buf, MSG_LEN)) != MSG_LEN)
        {
            printf("Write failed - %d\n", len);
            exit(2);
        }
    }
    printf("Pointers %d %d %d\n", queue->iptr, queue->optr, queue->len);
    queue_delete(queue);
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int i;
    int threaded_messages;
    int threaded_streams;

    threaded_messages = FALSE;
    threaded_streams = FALSE;
    for (i = 1;  i < argc;  i++)
    {
        if (strcmp(argv[i], "-m") == 0)
        {
            threaded_messages = TRUE;
            continue;
        }
        if (strcmp(argv[i], "-s") == 0)
        {
            threaded_streams = TRUE;
            continue;
        }
    }

    /* Test the basic functionality of the queueing code in stream and message modes */
    printf("Stream mode functional tests\n");
    functional_stream_tests();
    printf("Message mode functional tests\n");
    functional_message_tests();

    /* Run separate write and read threads for a while, to verify there are no locking
       issues. */
    if (threaded_streams)
    {
        printf("Stream mode threaded tests\n");
        threaded_stream_tests();
    }
    if (threaded_messages)
    {
        printf("Message mode threaded tests\n");
        threaded_message_tests();
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
