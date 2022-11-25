#include "UDHttp.h"
#include "mySD.h"


const char* ssid     = "dd-wrt";
const char* password = "0000000000";

File root;
//these callbacks will be invoked to read and write data to sdcard
//and process response
//and showing progress 
int responsef(uint8_t *buffer, int len){
  Serial.printf("%s\n", buffer);
  return 0;
}

int rdataf(uint8_t *buffer, int len){
  //read file to upload
  if (root.available()) {
    return root.read(buffer, len);
  }
  return 0;
}

int wdataf(uint8_t *buffer, int len){
  //write downloaded data to file
  return root.write(buffer, len);
}

void progressf(int percent){
  Serial.printf("%d\n", percent);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  Serial.print("Initializing SD card...");
  if (!SD.begin(32, 14, 12, 27)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  while(1){
    SD.remove("test.mp3");
    UDHttp udh;
    //open file on sdcard to write
    root = SD.open("test.mp3", FILE_WRITE);
    if (!root) {
       Serial.println("can not open file!");
       return;
    }
    //download the file from url
    int r = udh.download("http://192.168.1.3/audio.mp3", "audio.mp3", wdataf, progressf);
    if(r == -1)
    {
      Serial.println("error");
    }
    else
    {
      root.close();
      break;
    }
    root.close();
    Serial.printf("try downloading again\n");
    delay(1000);
  }
  Serial.printf("done downloading\n");
  {
    UDHttp udh;
    //open file on sdcard to read
    root = SD.open("test.pdf");
    if (!root) {
       Serial.println("can not open file!");
       return;
    }
    //upload downloaded file to local server
    udh.upload("http://192.168.1.107:80/upload/upload.php", "test.pdf", root.size(), rdataf, progressf, responsef);
    root.close();
    Serial.printf("done uploading\n");
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
