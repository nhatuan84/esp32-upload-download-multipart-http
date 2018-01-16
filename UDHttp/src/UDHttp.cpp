/*
    Tuan Nguyen (http://www.iotsharing.com) or (nha.tuan84@gmail.com)
*/

#include "UDHttp.h"
#include <base64.h>
#include <WiFiClientSecure.h>

#define HEADER "POST %s HTTP/1.1\r\n" \
                "Host: %s:%d\r\n"\
                "Connection: keep-alive\r\n"\
                "Accept: */*\r\n"\
                "Content-Length: %d\r\n"\
                "Expect: \r\n"\
                "Content-Type: multipart/form-data; boundary=------------------------%s\r\n\r\n"
#define OPEN  "--------------------------%s\r\n"\
                "Content-Disposition: form-data; name='data'; filename='%s'\r\n"\
                "Content-Type: application/octet-stream\r\n\r\n"
#define CLOSE   "\r\n--------------------------%s--\r\n"

#define GETR  "GET %s HTTP/1.1\r\nHost: %s:%d\r\nAccept: */*\r\n\r\n"

UDHttp::UDHttp(){ 
}

UDHttp::~UDHttp(){
}
//simple url parser
int UDHttp::simpleUrlParser(char *url, char *host, int &port){
    char *tmp = strstr(url, "http://");
    port = 80;
    char cport[6];
    char *tmp2;
    char *tmp3;
    if(tmp != NULL && host != NULL){
        tmp2 = tmp + strlen("http://");
        tmp = strchr(tmp2, '/');
        if(tmp != NULL){
            tmp3 = strchr(tmp2, ':');
            if(tmp3 == NULL) {
                int len = tmp - tmp2;
                if(len < HOST_LEN){
                    memcpy(host, tmp2, len);
                    return 0;
                }
                printf("increase HOST_LEN\n");
            } else {
                int len = tmp3 - tmp2;
                if(len < HOST_LEN){
                    memcpy(host, tmp2, len);
                    memset(cport, 0, 6);
                    memcpy(cport, tmp3+1, tmp- (tmp3+1));
                    port = atoi(cport);
                    return 0;
                }
                printf("increase HOST_LEN\n");
            }
        }
    }
    return -1;
}

void UDHttp::sendChunk(Client *client, uint8_t *buf, int len){
	int idx = 0;
	size_t result;
	while(len > 0){
		if(len < CHUNK_SIZE){
			result = client->write(&buf[idx], len);
		    len -= result;
		    idx += result;
		} else {
			result = client->write(&buf[idx], CHUNK_SIZE);
		    len -= result;
		    idx += result;
		}
	}
}

int UDHttp::upload(char *uploadUrlHandler, char *fileName, int sizeOfFile, DataCb dataCb, ProgressCb progressCb, DataCb responseCb){
    char buf[HEADER_SIZE];
	char host[HOST_LEN];
	int port = 80;
	int contentLen;
	WiFiClient client;
	int result;
	int sent = 0; 

	if(dataCb == NULL ){
		printf("DataCb or ProgressCb is NULL!\n");
        return -1;
    }
	//gen key from file name using b64
	//String key = base64::encode(String(fileName)); //it make app crash -> fix later
    char *key = "aHR0cDovL3d3dy5pb3RzaGFyaW5nLmNvbQ==";
	//very simple url parser

	if((strlen(OPEN) + strlen(key) + strlen(fileName)) > HEADER_SIZE){
		printf("Increase HEADER_SIZE\n");
		return -1;
	}
	if((strlen(CLOSE) + strlen(key)) > HEADER_SIZE){
		printf("Increase HEADER_SIZE\n");
		return -1;
	}
	if((strlen(HEADER) + strlen(host) + strlen(key) + 20) > HEADER_SIZE){
		printf("Increase HEADER_SIZE\n");
		return -1;
	}
    memset(host, 0, HOST_LEN);
    if(simpleUrlParser(uploadUrlHandler, host, port) == -1){
        printf("url is wrong\n");
        return -1;
    }
    //calculate open
    memset(buf, 0, HEADER_SIZE);
	snprintf(buf, HEADER_SIZE, OPEN, key, fileName);
    contentLen = strlen(buf);
    //calculate close
    memset(buf, 0, HEADER_SIZE);
	snprintf(buf, HEADER_SIZE, CLOSE, key);
	// content-length
	contentLen = contentLen + strlen(buf) + sizeOfFile;
    //fill header
    memset(buf, 0, HEADER_SIZE);
	snprintf(buf, HEADER_SIZE, HEADER, uploadUrlHandler, host, port, contentLen, key);
	
	if (!client.connect(host, port)) {
        printf("Connection failed\n");
        return -1;
	}
    //send header
	sendChunk(&client, (uint8_t *)buf, strlen(buf));
    memset(buf, 0, HEADER_SIZE);
    //send open
	snprintf(buf, HEADER_SIZE, OPEN, key, fileName);
	sendChunk(&client, (uint8_t *)buf, strlen(buf));
    //send data
	do{
		result = dataCb((uint8_t *)buf, CHUNK_SIZE);
		sendChunk(&client, (uint8_t *)buf, result);
        if(progressCb != NULL){
            sent += result;
            progressCb(sent*100/sizeOfFile);
        }
	}while(result >0);
    memset(buf, 0, HEADER_SIZE);
	snprintf(buf, HEADER_SIZE, CLOSE, key);
	sendChunk(&client, (uint8_t *)buf, strlen(buf));
	memset(buf, 0, CHUNK_SIZE);
    //process response
    while (client.available() > 0){
        int result = client.read((uint8_t *)buf, CHUNK_SIZE);
		if(responseCb != NULL && result != -1){
            responseCb((uint8_t *)buf, result);
        }
	}
	return 0;
}

int UDHttp::download(char *downloadUrl, DataCb dataCb, ProgressCb progressCb){
    char buf[HEADER_SIZE];
    char host[HOST_LEN];
	int port = 80;
    WiFiClient client;
    char num[10];
    unsigned int bytes;
    unsigned int received = 0;
    char *tmp = NULL;
    char *data;
    unsigned int total;
    unsigned int len = strlen("\r\n\r\n");

    if(dataCb == NULL){
        printf("DataCb is NULL!\n");
        return -1;
    }
	if((strlen(GETR) + strlen(downloadUrl)) > HEADER_SIZE){
        printf("Increase HEADER_SIZE\n");
		return -1;
	}
    memset(buf, 0, HEADER_SIZE);
	snprintf(buf, HEADER_SIZE, GETR, downloadUrl, host, port);
    //parse url
    memset(host, 0, HOST_LEN);
    if(simpleUrlParser(downloadUrl, host, port) == -1){
        printf("url is wrong\n");
        return -1;
    }
	if (!client.connect(host, port)) {
        printf("Connection failed\n");
        return -1;
	}

    sendChunk(&client, (uint8_t *)buf, strlen(buf));
    memset(buf, 0, HEADER_SIZE);
    //read response
    do {
        if(client.available()){
            bytes = client.read((uint8_t *)&buf[received], HEADER_SIZE);
            if(bytes != -1){
                received += bytes;
                tmp = strstr(buf, "\r\n\r\n");
            }
        }
    } while (tmp == NULL);
    //parse header
    data = tmp;
    tmp = strstr(buf, "Content-Length: ");
    char *tmp2 = strstr(tmp, "\r\n");
    memset(num, 0, 10);
    int clen = strlen("Content-Length: ");
    memcpy(num, tmp+clen, tmp2-tmp-clen);
    total = atoi(num);
    //start downloading
    if(received > (data+len+1-buf)){
		dataCb((uint8_t *)(data+len), received - (data+len-buf));
		total = total - (received - (data+len-buf));
	}
    clen = total;
    received = 0;
	do {
		bytes = client.read((uint8_t *)buf, HEADER_SIZE);
        if(bytes != -1){
            received += bytes;
		    dataCb((uint8_t *)buf, bytes);
		    total = total - bytes;
            if(progressCb != NULL){
                progressCb(100*received/clen);
            }
        }
	} while (total > 0);
	return 0;	
}
