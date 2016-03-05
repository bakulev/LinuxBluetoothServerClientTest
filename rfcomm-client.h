#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int dynamic_bind_rc(int sock, struct sockaddr_rc *sockaddr, uint8_t *port);

int rfcomm_client(void);

char* hciscan(char *dest_addr);
