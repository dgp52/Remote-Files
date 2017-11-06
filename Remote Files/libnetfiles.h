//Name: Deepkumar Patel, NetID: dgp52
//Name: Viraj Patel, NetID: vjp60

#ifndef _LIBNET_FILES_H_
#define _LIBNET_FILES_H_

int netserverinit(char * hostname, int filemode);
int netopen(const char*pathname, int flags);
int netread(int fildes, void *buf, size_t nbyte);
int netwrite(int fildes, const void *buf, size_t nbyte);
int netclose(int fildes);

#endif