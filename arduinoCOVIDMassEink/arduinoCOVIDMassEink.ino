#include <PDM.h>
#include <SPI.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include "imagedata.h"

#define COLORED     0
#define UNCOLORED   1

unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8 
Epd epd;

// buffer to read samples into, each sample is 16-bits
short sampleBuffer[256];

// number of samples read
volatile int samplesRead;

void drawImage(const uint8_t* image_addr){
;//
}

void drawTest(void){
  paint.Clear(UNCOLORED);
  epd.SetFrameMemory(deephomebrew_logo);
  epd.DisplayFrame();
}

int pop_detection = 0;
bool smiling = false;
unsigned long smiletimer = 0;
unsigned long last_face = 0;

void setup() {
    Serial.begin(9600);
    if (epd.Init(lut_full_update) != 0) {
        Serial.print("e-Paper init failed");
        return;
    }    

    /** 
     *  there are 2 memory areas embedded in the e-paper display
     *  and once the display is refreshed, the memory area will be auto-toggled,
     *  i.e. the next action of SetFrameMemory will set the other memory area
     *  therefore you have to clear the frame memory twice.
     */
    epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
    epd.DisplayFrame();
    epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
    epd.DisplayFrame();

    delay(2000);

    if (epd.Init(lut_partial_update) != 0) {
        Serial.print("e-Paper init failed");
        return;
    }    
    
    // configure the data receive callback
    PDM.onReceive(onPDMdata);

    // optionally set the gain, defaults to 20
    PDM.setGain(30);
  
    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, 16000)) {
      Serial.println("Failed to start PDM!");
      while (1);
    }
  
    // show logo
    drawTest();
    delay(5000);

}

float vol = 0;

void loop() {
    float nvol = 0;
    int val;

    // wait for samples to be read
    if (samplesRead) {
      // print samples to the serial monitor or plotter
      for (int i = 0; i < samplesRead; i++) {
        auto analog = sampleBuffer[i];
        val = map(analog, -32768, 32767, 0, 1023);
        auto micline = abs(val - 512);
        nvol = max(micline, nvol);                     
      }
      // clear the read count
      samplesRead = 0;
    }    
    
    vol = (nvol + 1.0*vol)/2.0;

    //Serial.println(vol); 

    if(nvol > 200){
        pop_detection += 1;
        if(pop_detection > 2) {
            smiling = false;
            last_face = millis();
        }
    } else {
        if(pop_detection > 0 && pop_detection <= 2) {
            if(millis() > last_face + 500){
                smiling = true;
                smiletimer = millis() + 2000;
            }
        }
        pop_detection = 0;
    }

    if(millis() > smiletimer) smiling = false;

    if(smiling){
        epd.SetFrameMemory(mouth_smile);
    } else if(vol < 2){
        epd.SetFrameMemory(mouth_0);
    } else if(vol < 4){
        epd.SetFrameMemory(mouth_1);
    } else if(vol < 6){
        epd.SetFrameMemory(mouth_2);
    } else if(vol < 8){
        epd.SetFrameMemory(mouth_3);
    } else {
        epd.SetFrameMemory(mouth_4);
    }
    epd.DisplayFrame();
} 

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
