#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "bluetooth.h"

int bt_scan()
{
	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;
	int i;
	char addr[19] = { 0 };
	char name[248] = { 0 };

	/* retrieve the resource number of the first available Bluetooth adapter */
	dev_id = hci_get_route(NULL); 
	// int dev_id = hci_devid( "01:23:45:67:89:AB" );

	/* connection to the MCU on the specified local Bluetooth adapter,
	 * 'and not a connection to a remote Bluetooth device */
	sock = hci_open_dev( dev_id );
	if (dev_id < 0 || sock < 0) {
		perror("opening socket");
		exit(1);
	}

	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	/* performs a BT device discovery and returns a list of detected devices */
	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
	if( num_rsp < 0 ) perror("hci_inquiry");

	for (i = 0; i < num_rsp; i++) {
		ba2str(&(ii+i)->bdaddr, addr);
		memset(name, 0, sizeof(name));
		if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
			strcpy(name, "[unknown]");
		printf("%s  %s\n", addr, name);
	}

	free( ii );
	close( sock );
	return 0;
}

//int bt_recv()
//{
//	struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
//	char buf[1024] = { 0 };
//	int s, client, bytes_read;
//	socklen_t opt = sizeof(rem_addr);
//
//	// allocate socket
//	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
//
//	// bind socket to port 1 of the first available local bluetooth adapter
//	loc_addr.rc_family = AF_BLUETOOTH;
//	loc_addr.rc_bdaddr = *BDADDR_ANY;
//	loc_addr.rc_channel = (uint8_t) 1;
//	bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
//
//	// put socket into listening mode
//	listen(s, 1);
//
//	// accept one connection
//	client = accept(s, (struct sockaddr *)&rem_addr, &opt);
//
//	ba2str( &rem_addr.rc_bdaddr, buf );
//	fprintf(stderr, "accepted connection from %s\n", buf);
//	memset(buf, 0, sizeof(buf));
//
//	// read data from the client
//	bytes_read = read(client, buf, sizeof(buf));
//	if( bytes_read > 0 ) {
//		printf("received [%s]\n", buf);
//	}
//
//	// close connection
//	close(client);
//	close(s);
//	return 0;
//}

int* bt_open_conn()
{
	struct sockaddr_rc addr = { 0 };
	int sock, status;
	const char *dest = "20:13:06:03:02:00";
	int *pack = (int*)calloc(2, sizeof(int));

	if(pack == NULL)
		printf("calloc error");

	// allocate a socket
	sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	// set the connection parameters (who to connect to)
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;
	str2ba( dest, &addr.rc_bdaddr );

	// connect to server
	status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

	pack[0] = status;
	pack[1] = sock;

	return pack;
}


int bt_close_conn(int sock)
{
	close(sock);

	return 0;
}

int bt_send(int sock, int status, const char *cmd)
{
	// send a message
	if( status >= 0 )
		status = write(sock, cmd, strlen(cmd));

	if( status < 0 )
		perror("uh oh");

	sleep(1);
	return status;
}

////http://people.csail.mit.edu/rudolph/Teaching/Articles/BTBook.pdf
////http://stackoverflow.com/questions/14820004/bluetooth-pairing-in-c-bluez-on-linux
//
///* Request authentication */
//static void cmd_auth(int dev_id, int argc, char **argv)
//{
//	struct hci_conn_info_req *cr;
//	bdaddr_t bdaddr;
//	int opt, dd;
//
//	for_each_opt(opt, auth_options, NULL) {
//		switch (opt) {
//			default:
//				printf("%s", auth_help);
//				return;
//		}
//	}
//	helper_arg(1, 1, &argc, &argv, auth_help);
//
//	str2ba(argv[0], &bdaddr);
//
//	if (dev_id < 0) {
//		dev_id = hci_for_each_dev(HCI_UP, find_conn, (long) &bdaddr);
//		if (dev_id < 0) {
//			fprintf(stderr, "Not connected.\n");
//			exit(1);
//		}
//	}
//
//	dd = hci_open_dev(dev_id);
//	if (dd < 0) {
//		perror("HCI device open failed");
//		exit(1);
//	}
//
//	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
//	if (!cr) {
//		perror("Can't allocate memory");
//		exit(1);
//	}
//
//	bacpy(&cr->bdaddr, &bdaddr);
//	cr->type = ACL_LINK;
//	if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
//		perror("Get connection info failed");
//		exit(1);
//	}
//
//	if (hci_authenticate_link(dd, htobs(cr->conn_info->handle), 25000) < 0) {
//		perror("HCI authentication request failed");
//		exit(1);
//	}
//
//	free(cr);
//
//	hci_close_dev(dd);
//}
//
///* Activate encryption */
//static void cmd_enc(int dev_id, int argc, char **argv)
//{
//	struct hci_conn_info_req *cr;
//	bdaddr_t bdaddr;
//	uint8_t encrypt;
//	int opt, dd;
//
//	for_each_opt(opt, enc_options, NULL) {
//		switch (opt) {
//			default:
//				printf("%s", enc_help);
//				return;
//		}
//	}
//	helper_arg(1, 2, &argc, &argv, enc_help);
//
//	str2ba(argv[0], &bdaddr);
//
//	if (dev_id < 0) {
//		dev_id = hci_for_each_dev(HCI_UP, find_conn, (long) &bdaddr);
//		if (dev_id < 0) {
//			fprintf(stderr, "Not connected.\n");
//			exit(1);
//		}
//	}
//
//	dd = hci_open_dev(dev_id);
//	if (dd < 0) {
//		perror("HCI device open failed");
//		exit(1);
//	}
//
//	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
//	if (!cr) {
//		perror("Can't allocate memory");
//		exit(1);
//	}
//
//	bacpy(&cr->bdaddr, &bdaddr);
//	cr->type = ACL_LINK;
//	if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
//		perror("Get connection info failed");
//		exit(1);
//	}
//
//	encrypt = (argc > 1) ? atoi(argv[1]) : 1;
//
//	if (hci_encrypt_link(dd, htobs(cr->conn_info->handle), encrypt, 25000) < 0) {
//		perror("HCI set encryption request failed");
//		exit(1);
//	}
//
//	free(cr);
//
//	hci_close_dev(dd);
//}

/* EOF */
