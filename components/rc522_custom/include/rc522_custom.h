#ifndef _RC522_CUSTOM_H_
#define _RC522_CUSTOM_H_

#define RC522_CUSTOM_UID_BUFFER_SIZE 30

extern char rc522_custom_uid_buffer[RC522_CUSTOM_UID_BUFFER_SIZE];

void rc522_custom_init(void);

void rc522_custom_start(void);

void rc522_custom_pause(void);


#endif