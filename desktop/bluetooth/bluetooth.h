#ifndef SIMPLE_BLUETOOTH_H_
#define SIMPLE_BLUETOOTH_H_

// http://people.csail.mit.edu/albert/bluez-intro/x502.html

int bt_scan();
//int bt_recv();
int* bt_open_conn();
int bt_close_conn(int);
int bt_send(int, int, const char *);
//static void cmd_auth(int, int, char**);
//static void cmd_enc(int, int, char**);

#endif
