/*
    Tuan Nguyen (http://www.iotsharing.com) or (nha.tuan84@gmail.com)
*/

#ifndef UDHTTP_H
#define UDHTTP_H

#include <functional>
#include <WiFi.h>

#define HEADER_SIZE	500
#define CHUNK_SIZE	100
#define HOST_LEN    200
//callback that will be invoked whenever data is available
typedef int (*DataCb)(uint8_t *buffer, int len);
typedef void (*ProgressCb)(int percent);

class UDHttp
{
	private:
		void sendChunk(Client *client, uint8_t *buf, int len);
        int simpleUrlParser(char *url, char *host, int &port);
    public: 
        UDHttp();
        ~UDHttp();
        int upload(char *uploadUrlHandler, char *fileName, int sizeOfFile, DataCb dataCb, ProgressCb progressCb, DataCb responseCb);
        int download(char *downloadUrl, DataCb dataCb, ProgressCb progressCb);
};

#endif
