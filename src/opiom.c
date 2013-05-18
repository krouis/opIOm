/* 
 *
 * author: Khalifa Rouis
 * date: 12/02/2010
 * project name: opiom (open performance Input Output measurement tool)
 * version: 0.4
 *
 */
 /*
  * modified 20/07/2009
  *  -> changed the lazy write test function by the write through one.
  * modified 23/07/2009
  *  -> changed the direct read to a random buffered read (for test purposes).
  * modified 12/02/2010
  *  -> added the random direct read function
  *  -> added gengetopt parser 
  * updated 16/02/2010 :
  *   -> updated the name of the functions from functions.c
  * modified 17/02/2010
  *   -> deleted logutils.* the configuration files will be handled by 
  *      the CLI for now.
  *   -> functional output functions to be added soon 
  */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* exit status */
#include <sys/time.h> /* gettimeofday() */
#include <ctype.h>
#include <unistd.h>
#include "cmdline.h" /* command line interface generated by gengetopt*/
#include "functions.h" /* test functions */
#include "args.h" /* contains program's arguments structure */


/* the actual program name */
char* program_name;

int main (int argc, char **argv){ 
  int testfile_descriptor;
  struct gengetopt_args_info a;
  if (cmdline_parser(argc, argv, &a) != 0)
    exit(1) ;

  if(a.create_given)
  {
    testfile_descriptor = create_testfile(a.testfile_name_arg,a.testfile_size_arg);
    close(testfile_descriptor);
  }

  if(a.write_given)
  {
    if(a.random_given)
    {
      measure_random_synchrounous_write(a.testfile_name_arg, a.request_size_arg,
                                        a.requests_number_arg, a.testfile_size_arg);
    }
    else
    {
      printf("write through\n");
      measure_write_through(a.testfile_name_arg, a.request_size_arg, 
                            a.requests_number_arg);
    }
  }
  
  if(a.read_given)
  {
    if(a.random_given)
      measure_random_direct_read(a.testfile_name_arg,a.request_size_arg,
                                 a.requests_number_arg,a.testfile_size_arg);
    else
      measure_sequential_direct_read(a.testfile_name_arg,a.request_size_arg,
                                     a.requests_number_arg);
  }
  /* for the gengetopt structure (in cmdline.c) */
  cmdline_parser_free (&a);
  return EXIT_SUCCESS;
}