/*
 * Copyright (C) 2016-2017 Alexander Saprykin <xelfium@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "plibsys.h"
#include "ptestmacros.h"

#include <string.h>

P_TEST_MODULE_INIT ();

#define PRWLOCK_TEST_STRING_1 "This is a test string."
#define PRWLOCK_TEST_STRING_2 "Ouh, yet another string to check!"

static PRWLock *         test_rwlock        = NULL;
static volatile pboolean is_threads_working = FALSE;
static volatile pint     writers_counter    = 0;
static pchar             string_buf[50];

extern "C" ppointer pmem_alloc (psize nbytes)
{
	P_UNUSED (nbytes);
	return (ppointer) NULL;
}

extern "C" ppointer pmem_realloc (ppointer block, psize nbytes)
{
	P_UNUSED (block);
	P_UNUSED (nbytes);
	return (ppointer) NULL;
}

extern "C" void pmem_free (ppointer block)
{
	P_UNUSED (block);
}

static void * reader_thread_func (void *data)
{
	P_UNUSED (data);

	pint counter = 0;

	while (p_atomic_int_get (&writers_counter) == 0)
		p_uthread_sleep (10);

	while (is_threads_working == TRUE) {
		p_uthread_sleep (10);

		if (p_rwlock_reader_trylock (test_rwlock) == FALSE) {
			if (p_rwlock_reader_lock (test_rwlock) == FALSE)
				p_uthread_exit (-1);
		}

		if (strcmp (string_buf, PRWLOCK_TEST_STRING_1) != 0 &&
		    strcmp (string_buf, PRWLOCK_TEST_STRING_2) != 0) {
			p_rwlock_reader_unlock (test_rwlock);
			p_uthread_exit (-1);
		}

		if (p_rwlock_reader_unlock (test_rwlock) == FALSE)
			p_uthread_exit (-1);

		++counter;
	}

	p_uthread_exit (counter);

	return NULL;
}

static void * writer_thread_func (void *data)
{
	pint string_num = PPOINTER_TO_INT (data);
	pint counter    = 0;

	while (is_threads_working == TRUE) {
		p_uthread_sleep (10);

		if (p_rwlock_writer_trylock (test_rwlock) == FALSE) {
			if (p_rwlock_writer_lock (test_rwlock) == FALSE)
				p_uthread_exit (-1);
		}

		memset (string_buf, 0, sizeof (string_buf));

		if (string_num == 1)
			strcpy (string_buf, PRWLOCK_TEST_STRING_1);
		else
			strcpy (string_buf, PRWLOCK_TEST_STRING_1);

		if (p_rwlock_writer_unlock (test_rwlock) == FALSE)
			p_uthread_exit (-1);

		++counter;

		p_atomic_int_inc ((&writers_counter));
	}

	p_uthread_exit (counter);

	return NULL;
}

P_TEST_CASE_BEGIN (prwlock_nomem_test)
{
	p_libsys_init ();

	PMemVTable vtable;

	vtable.free    = pmem_free;
	vtable.malloc  = pmem_alloc;
	vtable.realloc = pmem_realloc;

	P_TEST_CHECK (p_mem_set_vtable (&vtable) == TRUE);

	P_TEST_CHECK (p_rwlock_new () == NULL);

	p_mem_restore_vtable ();

	p_libsys_shutdown ();
}
P_TEST_CASE_END ()

P_TEST_CASE_BEGIN (prwlock_bad_input_test)
{
	p_libsys_init ();

	P_TEST_CHECK (p_rwlock_reader_lock (NULL) == FALSE);
	P_TEST_CHECK (p_rwlock_reader_trylock (NULL) == FALSE);
	P_TEST_CHECK (p_rwlock_reader_unlock (NULL) == FALSE);
	P_TEST_CHECK (p_rwlock_writer_lock (NULL) == FALSE);
	P_TEST_CHECK (p_rwlock_writer_trylock (NULL) == FALSE);
	P_TEST_CHECK (p_rwlock_writer_unlock (NULL) == FALSE);
	p_rwlock_free (NULL);

	p_libsys_shutdown ();
}
P_TEST_CASE_END ()

P_TEST_CASE_BEGIN (prwlock_general_test)
{
	p_libsys_init ();

	test_rwlock = p_rwlock_new ();

	P_TEST_REQUIRE (test_rwlock != NULL);

	is_threads_working = TRUE;
	writers_counter    = 0;

	PUThread *reader_thr1 = p_uthread_create ((PUThreadFunc) reader_thread_func,
						  NULL,
						  TRUE);

	PUThread *reader_thr2 = p_uthread_create ((PUThreadFunc) reader_thread_func,
						  NULL,
						  TRUE);

	PUThread *writer_thr1 = p_uthread_create ((PUThreadFunc) writer_thread_func,
						  NULL,
						  TRUE);

	PUThread *writer_thr2 = p_uthread_create ((PUThreadFunc) writer_thread_func,
						  NULL,
						  TRUE);

	P_TEST_REQUIRE (reader_thr1 != NULL);
	P_TEST_REQUIRE (reader_thr2 != NULL);
	P_TEST_REQUIRE (writer_thr1 != NULL);
	P_TEST_REQUIRE (writer_thr2 != NULL);

	p_uthread_sleep (10000);

	is_threads_working = FALSE;

	P_TEST_CHECK (p_uthread_join (reader_thr1) > 0);
	P_TEST_CHECK (p_uthread_join (reader_thr2) > 0);
	P_TEST_CHECK (p_uthread_join (writer_thr1) > 0);
	P_TEST_CHECK (p_uthread_join (writer_thr2) > 0);

	p_uthread_unref (reader_thr1);
	p_uthread_unref (reader_thr2);
	p_uthread_unref (writer_thr1);
	p_uthread_unref (writer_thr2);

	p_rwlock_free (test_rwlock);

	p_libsys_shutdown ();
}
P_TEST_CASE_END ()

P_TEST_SUITE_BEGIN()
{
	P_TEST_SUITE_RUN_CASE (prwlock_nomem_test);
	P_TEST_SUITE_RUN_CASE (prwlock_bad_input_test);
	P_TEST_SUITE_RUN_CASE (prwlock_general_test);
}
P_TEST_SUITE_END()
