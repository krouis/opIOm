/*
 * author: Khalifa Rouis
 * date: 02/06/2009
 * project name: opiom (open performance IO measurement tool)
 * version: 0.5.4
 * updated 20/07/2009 :
 *   -> add of the write through function (using O_SYNC)
 * updated xx/xx/xxxx:
 *   -> add random address generating fuction
 *   -> add of random direct and buffered read functions
 * updated 16/02/2010 :
 *   -> change coding style to be GNU compatible
 * 
 * 22.02.2010
 * I know I shouldn't be doing this, but for urgent testing
 * sake, I'm taking away the O_SYNC flags from the writes
 */
 
#include <stdio.h>
#include <unistd.h> /* access */
#include <fcntl.h> /* creat, fcntl */
#include <stdlib.h>
#include <sys/types.h> /* O_DIRECT */
#include <sys/stat.h>
#include <sys/time.h> /* gettimeofday */
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/mman.h>
#include "functions.h"
#include "args.h"

#define MAX_UNSIGNED_CHAR_VAL 255


/* 
 * verifies the existence of the test
 * file testfile_name
 * returns 0 if it exists
 * returns -1 otherwise
 */
int 
verify_testfile(const char *testfile_name)
{
  return (access(testfile_name,F_OK));
}

/*
 * createTestFile() creates a test file
 * with testfile_name as file name and size as the
 * file's size in KB.
 * if size <= 0 then the function will
 * only create the file.
 * it returns 0 if the test file is
 * successfully created and -1 otherwise
 */
int 
create_testfile(const char *testfile_name,int testfile_size)
{
  int testfile_descriptor;
  testfile_descriptor = creat(testfile_name,defmode);
  if (testfile_size>0)
    fill_testfile(testfile_descriptor,testfile_size);
  if(testfile_descriptor<0)
    return(-1);
  close(testfile_descriptor);
  return EXIT_SUCCESS;
}

int
clear_testfile(const char *testfile_name)
{
  return (unlink(testfile_name));
}

int
open_testfile(const char *testfile_name)
{
  int testfile_descriptor;
  testfile_descriptor = open(testfile_name,O_RDWR);
  return EXIT_SUCCESS;
}

/*
 * prints to screen the golbal performances.
 * calculates the bandwidth.
 */
int
print_global_performance(double total_time_elapsed, int total_size,
                         int request_size)
{
  double throughput;
  /* throughput is the throughput in bytes per second */
  throughput=(total_size)/(total_time_elapsed/1000);
  /* 
   * the overall performances display
   * to be toggled with a parameter
   * the throughput is displayed in MBps
   */
  fprintf(stderr,"%d\t%f\n",request_size,(throughput/1024)/1024);
  return EXIT_SUCCESS;
}

/*
 * generates a random offset to be used
 * during the random access to the testfile
 * the generated number is lesser than limit
 * and always a multiple of 64.
 */
int
generate_random_address(int limit)
{
  int result;
  struct timeval reference_time;
  gettimeofday(&reference_time,NULL);
  srand(reference_time.tv_usec);
  result = (int)((int)((rand() / (double) RAND_MAX)*limit) / 512) * 512;
  return(result);
}

/*
 * reading test to measure the reading
 * performance of the file pointed by
 * testfile_name.
 * This function basically operates
 * readings with given parameters (testfile_name 
 * for the test file name, request_size for the request
 * size, requests_number for the number of requests)
 * using the read() primitive placed
 * between two time markers(gettimeofday)
 * that calculate the elapsed time between
 * them.
 * In order to get the real transfert time
 * we need to ignore the Kernel's IO caching
 * system hence the use of the O_DIRECT flag.
 * */
int
measure_sequential_direct_read(const char *testfile_name, int request_size,
                               int requests_number)
{
  ssize_t bytes_read,local_request_size,local_requests_number,total_size=0;
  int i,testfile_descriptor; /* i as a counter and testfile_descriptor is the file descriptor */
  struct timeval start_timer; /* start time marker */
  struct timeval stop_timer; /* stop time marker */
  double time_elapsed;
  double total_time_elapsed = 0;
  char *read_buffer;
  int buf;


  /* 
   * initializing the local parameters 
   * if the parameters given by the user
   * are incorrect or missing, default ones
   * will be used 
   * */ 

  if((local_request_size=request_size*1024*sizeof(char))<=0)
    local_request_size=default_request_size*1024*sizeof(char);
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;

 /* some *NIX Flavours don't support O_DIRECT
  * (notably DARWIN) so if we execute the program
  * under DARWIN or MAC OS, use F_NOCACHE instead
  * we should (or so says Master Yoda).
  */

#ifdef __darwin__ 
  testfile_descriptor = open(testfile_name,O_RDONLY);
  if(testfile_descriptor<0)
  { 
    perror("open");
    return (EXIT_FAILURE);
  }
  fcntl(testfile_descriptor,F_NOCACHE,1);
#else /* NOT DARWIN */
  testfile_descriptor = open(testfile_name,O_RDONLY|O_DIRECT);
  if(testfile_descriptor<0)
  {
    perror("open");
    return (EXIT_FAILURE);
  }
#endif

#ifdef __linux__
  buf = posix_memalign((void*)&read_buffer, 512, local_request_size);
  if (read_buffer == NULL)
  {
    perror("malloc");
    free(read_buffer);
    close(testfile_descriptor);
    return (EXIT_FAILURE);
  }
#else /* other operating systems don't need memory alignement with O_DIRECT */
  (void) buf; /* pacify the unused variable warning */
  read_buffer = (char*)malloc(local_request_size);
#endif

  for(i=0;i<requests_number;i++)
  {
    gettimeofday(&start_timer,NULL); /* starts the start_timer time marker */
    bytes_read = read(testfile_descriptor,read_buffer,local_request_size); /* actual reading operation */
    gettimeofday(&stop_timer,NULL); /* stops the stop_timer time marker */
    if(bytes_read>0)
    {
      /* time conversion and total values calculation */
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0; /* seconds to ms */
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) /1000.0; /* microsec to ms*/
      total_time_elapsed+=time_elapsed; /* in ms */
      total_size+=bytes_read;
      /* data display will be very soon replaced by
       * a gnuplot data file 
       */
      printf("%d\t%f\n",i,time_elapsed);
      if(bytes_read<local_request_size) /* end of file reached */
        break;
    }
    else
    {
      perror("read");
      fprintf(stderr,"Reading problem, abandoning reading measurment\n");
      break;
    }
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fsync %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(close(testfile_descriptor)==-1)
  {
    perror("close");
    free(read_buffer);
    return EXIT_FAILURE;
  }  
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#close %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(system("umount /mnt/flash")==-1)
  {
    perror("system");
    free(read_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#umount %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  printf("#total_size %d Bytes\n",total_size);
  printf("#total_time_elapsed %f ms\n",total_time_elapsed);
  print_global_performance(total_time_elapsed, total_size, request_size); 
  free(read_buffer);
  return EXIT_SUCCESS;
}

int 
measure_random_direct_read(const char *testfile_name,int request_size, 
                           int requests_number,int limit)
{
  ssize_t bytes_read,local_request_size,local_requests_number,total_size=0;
  int i,buf,testfile_descriptor; 
  /* i as a counter and testfile_descriptor is the file descriptor */
  struct timeval start_timer; /* start time marker */
  struct timeval stop_timer; /* stop time marker */
  double time_elapsed;
  double total_time_elapsed = 0;
  off_t offset;
  char *read_buffer;
  int actual_offset;
  /*
   * initializing the local parameters
   * if the parameters given by the user
   * are incorrect or missing, default ones
   * will be used
   * the local_request_size is in bytes
   * */

  if((local_request_size=request_size*1024*sizeof(char))<=0)
    local_request_size=default_request_size*1024*sizeof(char);
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;


 /* some *NIX Flavours don't support O_DIRECT
  * (notably DARWIN) so if we execute the program
  * under DARWIN or MAC OS, use F_NOCACHE instead
  * we should (or so says Master Yoda).
  */

#ifdef __darwin__ 
  testfile_descriptor = open(testfile_name,O_RDONLY);
  if(testfile_descriptor<0)
  { 
    perror("open");
    return (EXIT_FAILURE);
  }
  fcntl(testfile_descriptor,F_NOCACHE,1);
#else /* NOT DARWIN */
  testfile_descriptor = open(testfile_name,O_RDONLY|O_DIRECT);
  if(testfile_descriptor<0)
  {
    perror("open");
    return (EXIT_FAILURE);
  }
#endif

#ifdef __linux__
  buf = posix_memalign((void*)&read_buffer, 512, local_request_size);
  if (read_buffer == NULL)
  {
    perror("malloc");
    free(read_buffer);
    close(testfile_descriptor);
    return (EXIT_FAILURE);
  }
#else /* other operating systems don't need memory alignement with O_DIRECT */
  (void) buf; /* pacify the unused variable warning */
  read_buffer = (char*)malloc(local_request_size);
#endif
  for(i=0;i<local_requests_number;i++)
  {  	
    offset = generate_random_address((limit*1024)-local_request_size);
    /* if the offset is < 0 we have a problem .. there is no point in
     * finishing the measure */
    if(offset<0)
    {
      perror("generate_random_address");
      free(read_buffer);
      close(testfile_descriptor);
      return (EXIT_FAILURE);	
    }
    actual_offset = lseek(testfile_descriptor, offset,SEEK_SET);
    if(actual_offset<0)
    { 
      perror("lseek");
      free(read_buffer);
      close(testfile_descriptor);
      return (EXIT_FAILURE);
    }
    bytes_read=0; /*init bytes_read*/
    gettimeofday(&start_timer,NULL); /* starts the start_timer time marker */
    bytes_read = read(testfile_descriptor,read_buffer,local_request_size); /* actual reading operation */
    gettimeofday(&stop_timer,NULL); /* stops the stop_timer time marker */
    if(bytes_read>=0)
    {
      /* time conversion and total values calculation */
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0; /* seconds to ms */
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) /1000.0; /* microsec to ms*/
      total_time_elapsed+=time_elapsed; /* in ms */
      total_size+=bytes_read;
      /* 
       * data display will be very soon replaced by
       * a gnuplot data file
       */
      printf("%d\t%f\t%d\n",i,time_elapsed,(int)offset);
      /*since the reading offset is random we don't check the EOF anymore.*/
    }
    else
    {
      perror("read");
      close(testfile_descriptor);
      free(read_buffer);
      return -1;
    }
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fsync %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(close(testfile_descriptor)==-1)
  {
    perror("close");
    free(read_buffer);
    return EXIT_FAILURE;
  }  
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#close %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(system("umount /mnt/flash")==-1)
  {
    perror("system");
    free(read_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#umount %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  printf("#total_size %d Bytes\n",total_size);
  printf("#total_time_elapsed %f ms\n",total_time_elapsed);
  print_global_performance(total_time_elapsed, total_size, request_size); 
  free(read_buffer);
  return EXIT_SUCCESS;
}

/*
 * tests the read performance while seeking random
 * addresses into the file.
 *
 */

int
measure_random_buffered_read(const char *testfile_name,int request_size,
                             int requests_number,int limit)
{
  ssize_t bytes_read,local_request_size,local_requests_number,total_size=0;
  int i,testfile_descriptor; /* i as a counter and testfile_descriptor is the file descriptor */
  struct timeval start_timer; /* start time marker */
  struct timeval stop_timer; /* stop time marker */
  double time_elapsed;
  double total_time_elapsed = 0;
  char *buff;
  off_t offset;

  /*
   * initializing the local parameters
   * if the parameters given by the user
   * are incorrect or missing, default ones
   * will be used
   * */

  if((local_request_size=request_size*1024*sizeof(char))<=0)
    local_request_size=default_request_size*1024*sizeof(char);
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;

  testfile_descriptor = open(testfile_name,O_RDONLY);
  if(testfile_descriptor<0)
  {
    perror("open");
    return (EXIT_FAILURE);
  }

  buff=(char*)malloc(local_request_size);

  for(i=0;i<requests_number;i++)
  {
    offset = generate_random_address(limit);
    gettimeofday(&start_timer,NULL); /* starts the start_timer time marker */
    if((lseek(testfile_descriptor, offset,SEEK_SET))<0) /* seek operation */
    {
      perror("lseek");
      free(buff);
      close(testfile_descriptor);
      return(EXIT_FAILURE);
    }
    bytes_read = read(testfile_descriptor,buff,local_request_size); /* actual reading operation */
    gettimeofday(&stop_timer,NULL); /* stops the stop_timer time marker */
    if(bytes_read>0)
    {
      /* time conversion and total values calculation */
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0; /* seconds to ms */
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) /1000.0; /* microsec to ms*/
      total_time_elapsed+=time_elapsed; /* in ms */
      total_size+=bytes_read;
      /* data display will be very soon replaced by
       * a gnuplot data file
       */
      printf("%d\t%f\n",i,time_elapsed);
      if(bytes_read<local_request_size) /* end of file reached */
        break;
    }
    else
    {
      perror("read");
      fprintf(stderr,"Reading problem, abandoning reading measurment\n");
      break;
    }
  }
  gettimeofday(&start_timer,NULL);
  if(close(testfile_descriptor)==-1)
  {
    perror("close");
    free(buff);
    return EXIT_FAILURE;
  }  
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#close %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(system("umount /mnt/flash")==-1)
  {
    perror("system");
    free(buff);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#umount %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  printf("#total_size %d Bytes\n",total_size);
  printf("#total_time_elapsed %f ms\n",total_time_elapsed);
  print_global_performance(total_time_elapsed, total_size, request_size); 
  free(buff);
  return EXIT_SUCCESS;
}


/*
 * tests the write through performances
 * of the device. Uses O_SYNC flag.
 * more comments to be written soon ..
 */

int 
measure_write_through(const char *testfile_name, int request_size,
                      int requests_number)
{
  ssize_t bytes_written,local_request_size,local_requests_number,total_size=0;
  int i,testfile_descriptor;
/*  int j; */ /* uncomment after tests 15032010 */
  char *write_buffer;
  struct timeval start_timer;
  struct timeval stop_timer;
  double time_elapsed,total_time_elapsed=0;

  if((local_request_size=request_size*1024)<=0)
    local_request_size=default_request_size*1024;
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;

/*  testfile_descriptor = open(testfile_name,O_WRONLY|O_SYNC);*/
  testfile_descriptor = open(testfile_name,O_WRONLY|O_CREAT,0644);
  if(testfile_descriptor<0)
  {
    perror("open");
    return (EXIT_FAILURE);
  }
  write_buffer = (char*) calloc (local_request_size,sizeof(char)); /* test return value !*/ 
  for(i=0;i<local_requests_number;i++)
  {
    /* intialize the write_buffer with random data */
/*    srand((unsigned int)time(NULL));
    for (j = 0; j < local_request_size; j++)
      write_buffer[j] = (unsigned char)(rand() % (MAX_UNSIGNED_CHAR_VAL + 1));*/ /* uncomment after tests 15032010 */
    /* timed write operation */
    gettimeofday(&start_timer,NULL);
    bytes_written = write(testfile_descriptor,write_buffer,local_request_size*sizeof(char));
    gettimeofday(&stop_timer,NULL);
    if(bytes_written>0) /* test =:=lqs */
    {
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
      total_time_elapsed+=time_elapsed;
      total_size+=bytes_written;
      printf("%d\t%f\n",i,time_elapsed);
    }
    else
    {
      perror("write");
      fprintf(stderr,"Writing problem, abandoning writing measurment\n");
      break;
    }
  }
  gettimeofday(&start_timer,NULL);
  if(fflush(NULL)!=0)
  {
    perror("fflush");
    close(testfile_descriptor);
    free(write_buffer);
    return EXIT_FAILURE;
  }
/*  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fflush %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(fsync(testfile_descriptor)==-1)
  {
    perror("fsync");
    close(testfile_descriptor);
    free(write_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fsync %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(close(testfile_descriptor)==-1)
  {
    perror("close");
    free(write_buffer);
    return EXIT_FAILURE;
  }  
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#close %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(system("umount /mnt/flash")==-1)
  {
    perror("system");
    free(write_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#umount %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed; */ /* should be deleted after the tests 15032010 */
  printf("#local_request_size %d Bytes\n",local_request_size); /* delete after tests 15032010 */
  printf("#total_size %d Bytes\n",total_size);
  printf("#total_time_elapsed %f ms\n",total_time_elapsed);
  close(testfile_descriptor); 
  print_global_performance(total_time_elapsed, total_size, request_size); 
  free(write_buffer);
  return EXIT_SUCCESS;
}



int
measure_lazy_write(const char *testfile_name, int request_size,
                   int requests_number)
{
  ssize_t bytes_written,local_request_size,local_requests_number,total_size=0;
  int i,testfile_descriptor;
  char *buffer;
  struct timeval start_timer;
  struct timeval stop_timer;
  double time_elapsed,total_time_elapsed=0;

  if((local_request_size=request_size*1024*sizeof(char))<=0)
    local_request_size=default_request_size*1024*sizeof(char);
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;

  testfile_descriptor = open(testfile_name,O_WRONLY);
  if(testfile_descriptor<0)
    return (EXIT_FAILURE);
  buffer = (char*) calloc (local_request_size,sizeof(char));
  for(i=0;i<local_requests_number;i++)
  {
    gettimeofday(&start_timer,NULL);
    bytes_written = write(testfile_descriptor,buffer,local_request_size*sizeof(char));
    gettimeofday(&stop_timer,NULL);
    if(bytes_written>0)
    {
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
      total_time_elapsed+=time_elapsed;
      total_size+=bytes_written;
      printf("%d\t%f\n",i,time_elapsed);
    }
    else
    {
      perror("write");
      fprintf(stderr,"Writing problem, abandoning writing measurment\n");
      break;
    }
  }
  print_global_performance(total_time_elapsed, total_size, request_size);
  close(testfile_descriptor);
  free(buffer);
  return EXIT_SUCCESS;
}

/* simple test to measure a reference 
 * umount time
 */
int
measure_umount_time(void)
{
  int system_return;
  struct timeval start_timer;
  struct timeval stop_timer;
  double time_elapsed=0;
  system_return = system("mount -t vfat /dev/sdc1 /mnt/flash");
  if (system_return == -1)
  {
    perror("system");
    return EXIT_FAILURE;
  }
  sleep(10);
  gettimeofday(&start_timer,NULL);
  system_return = system("umount /mnt/flash");
  gettimeofday(&stop_timer,NULL);
  if (system_return != -1)
  {
    time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
    time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
    printf("\numount : %f ms\n",time_elapsed); 
  }
  else
  {
    perror("system");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
/*
 * measure the random synchronous write performance
 * of the device. Uses O_SYNC flag.
 */

int 
measure_random_synchrounous_write(const char *testfile_name, int request_size,
                                  int requests_number, int limit)
{
  ssize_t bytes_written,local_request_size,local_requests_number,total_size=0;
  int i,j,testfile_descriptor;
  char *write_buffer;
  struct timeval start_timer;
  struct timeval stop_timer;
  double time_elapsed,total_time_elapsed=0;
  int generated_offset, actual_offset;
/*  int buf;*/

  if((local_request_size=request_size*1024*sizeof(char))<=0)
    local_request_size=default_request_size*1024*sizeof(char);
  if((local_requests_number=requests_number)<=0)
    local_requests_number=default_requests_number;

/*  testfile_descriptor = open(testfile_name,O_WRONLY|O_DIRECT);*/
/*  testfile_descriptor = open(testfile_name,O_WRONLY|O_SYNC);*/
  testfile_descriptor = open(testfile_name,O_WRONLY);
  if(testfile_descriptor<0)
  {
    perror("open");
    return (EXIT_FAILURE);
  }
/*  buf = posix_memalign((void*)&write_buffer, 512, local_request_size);
  if (write_buffer == NULL)
  {
    perror("posix_memalign");
    free(write_buffer);
    close(testfile_descriptor);
    return (EXIT_FAILURE);
  }*/
  /* write_buffer contains the data to be inserted in the file */
  write_buffer = (char*) malloc (local_request_size);
  if(write_buffer==NULL)
  {
    perror("malloc");
    close(testfile_descriptor);
    return (EXIT_FAILURE);
  }
  for(i=0;i<local_requests_number;i++)
  {
    generated_offset = generate_random_address((limit*1024)-local_request_size);
    /* if the offset is < 0 we have a problem .. there is no point in
     * finishing the measure */
    if(generated_offset<0)
    {
      perror("generate_random_address");
      free(write_buffer);
      close(testfile_descriptor);
      return (EXIT_FAILURE);	
    }
    actual_offset = lseek(testfile_descriptor, generated_offset,SEEK_SET);
    if(actual_offset<0)
    { 
      perror("lseek");
      free(write_buffer);
      close(testfile_descriptor);
      return (EXIT_FAILURE);
    }
    /* intialize the write_buffer with random data */
    srand((unsigned int)time(NULL));
    for (j = 0; j < local_request_size; j++)
      write_buffer[j] = 'C';
/*      write_buffer[j] = (unsigned char)(rand() % (MAX_UNSIGNED_CHAR_VAL + 1));*/
    /* timed write operation */
    gettimeofday(&start_timer,NULL);
    bytes_written = write(testfile_descriptor,write_buffer,local_request_size);
    gettimeofday(&stop_timer,NULL);
    if(bytes_written>0)
    {
      time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
      time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
      total_time_elapsed+=time_elapsed;
      total_size+=bytes_written;
      printf("%d\t%f\t%d\n",i,time_elapsed,generated_offset);
    }
    else
    {
      perror("write");
      break;
    }
  }
  gettimeofday(&start_timer,NULL);
  if(fflush(NULL)!=0)
  {
    perror("fflush");
    close(testfile_descriptor);
    free(write_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fflush %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(fsync(testfile_descriptor)==-1)
  {
    perror("fsync");
    close(testfile_descriptor);
    free(write_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#fsync %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(close(testfile_descriptor)==-1)
  {
    perror("close");
    free(write_buffer);
    return EXIT_FAILURE;
  }  
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#close %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  gettimeofday(&start_timer,NULL);
  if(system("umount /mnt/flash")==-1)
  {
    perror("system");
    free(write_buffer);
    return EXIT_FAILURE;
  }
  gettimeofday(&stop_timer,NULL);
  time_elapsed = (stop_timer.tv_sec - start_timer.tv_sec) * 1000.0;
  time_elapsed += (stop_timer.tv_usec - start_timer.tv_usec) / 1000.0;
  printf("#umount %f\n",time_elapsed);
  total_time_elapsed+=time_elapsed;
  printf("#total_size %d Bytes\n",total_size);
  printf("#total_time_elapsed %f ms\n",total_time_elapsed);
  print_global_performance(total_time_elapsed, total_size, request_size); 
  free(write_buffer);
  return EXIT_SUCCESS;
}



/*
 * fillFile() fills a file identified by testfile_descriptor
 * with size KB
 * it returns the file discriptor if it succeeds
 * -1 otherwise
 */
/*
 * TODO: if the filesize is big,
 * the writing buffer should be
 * split into parts and the writing
 * performed into a loop
 * in order to prevent memory overflow
 * 15/05: done
 * otherwise we can't create 2GB testfiles
 * fixing the arbitrary limit to 50 MB
 */
int
fill_testfile(int testfile_descriptor, int testfile_size)
{
  char *zero;
  int i;
  int parts;
  parts = (testfile_size/51200); /* number of writing requests */
  if(parts<1) parts=1; /* if the total size is < to 50MB */
  zero = (char*) calloc((testfile_size/parts)*1024,sizeof(char));
  for(i=0;i<(testfile_size/parts)*1024;i++)
    zero[i]='A';
  for(i=0;i<parts;i++)
  {
    if(write(testfile_descriptor, (char*)zero,((testfile_size/parts)*1024)*sizeof(char))<0)
    {
      perror("write");
      free(zero);
      return -1;
    }
  }
  free(zero);
  return testfile_descriptor;
}
