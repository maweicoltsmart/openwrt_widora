#ifndef __MJ_MODBUS_SERVER_H__
#define __MJ_MODBUS_SERVER_H__

#define SERVER_ID		1

void *mjmodbus_server_routin(void *data);
void *mjmodbus_slave485_routin(void *data);
void *mjmodbus_slave232_routin(void *data);

#endif
