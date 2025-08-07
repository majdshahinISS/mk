/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Lukas Grützmacher <lg2@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#include <stdio.h>
#include <unistd.h>
#include "thread_test.h"

typedef void (* TEST_FUCTION)(void);

typedef struct 
{
  //int n;
  char *str;
  TEST_FUCTION test_func;
} Options;

#define MAX_OPTIONS 2

Options options[MAX_OPTIONS] = {
  { "For Thread test Press \'a\'", run_thread_test },
  { "End", NULL }
};



int
main(void)
{
  printf("myapp: Hello World! , using main function\n");
  
  for (size_t i = 0; i < MAX_OPTIONS; i++)
  {
    puts(options[i].str);
  }
  

  char c = '\0';
  c = getc(stdin);
  if (( c - 'a' < MAX_OPTIONS ) && (c - 'a') >= 0)
  {
    if (options[c - 'a'].test_func != NULL)
    {
      printf("myapp: Running test function for option %c\n", c);
      options[c - 'a'].test_func();
    }
    else
    {
      printf("myapp: No test function for option %c\n", c);
    }

  }
  else
  {
    printf("myapp: Invalid option\n");
  }
  printf("myapp: Exiting main function\n");

  
  return 0;
}
