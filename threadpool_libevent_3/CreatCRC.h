#ifndef _CREATCRC_F1_
#define _CREATCRC_F1_
#include <stdbool.h>


#define NICU_FREE(s)    do { if (s) { free(s); s=NULL; } } while(0)


unsigned short  CreateCRCCode(char* myBufferBytes,int len);
bool is_Validate_CRCCode_Of_CommunicateData_Ok(unsigned char * BufferHeadByte,unsigned char * BufferBodyBytes);
bool is_Validate_CRCCode_Of_MainOrBackRole_Ok(unsigned char * BufferHeadByte,unsigned char * BufferBodyBytes);
bool is_Validate_CRCCode_Of_UDPClientData_Ok(unsigned char * BufferOfUDPClient,int revdatalength);


#endif
