/* Host-test shim only. This does not emulate Watt-32 or a packet driver. */
#include <sys/select.h>
#include <unistd.h>
#define sock_init() 0
#define tcp_tick(socket_pointer) ((void)0)
#define closesocket close
