#ifndef FUNCTIONS_H
#define FUNCTIONS_H

extern int verify_testfile(const char* testfile_name);
extern int create_testfile(const char* testfile_name,int testfile_size);
extern int clear_testfile(const char* testfile_name);
extern int open_testfile(const char* testfile_name);
extern int print_global_performance(double total_time_elapsed, int total_size, int request_size);
extern int generate_random_address(int limit);
extern int measure_sequential_direct_read(const char* testfile_name, int request_size, int requests_number);
extern int measure_random_direct_read(const char* testfile_name,int request_size, int requests_number,int limit);
extern int measure_random_buffered_read(const char* testfile_name,int request_size, int requests_number,int limit);
extern int measure_write_through(const char* testfile_name, int request_size, int requests_number);
extern int measure_lazy_write(const char* testfile_name, int request_size, int requests_number);
extern int measure_random_synchrounous_write(const char *testfile_name, int request_size, int requests_number, int limit);
extern int fill_testfile(int testfile_descriptor, int testfile_size);
extern int measure_umount_time(void);
#endif /* FUNCTIONS_H */
