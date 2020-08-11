#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <gamma.h>

#include <PDM.h>

// buffer to read samples into, each sample is 16-bits
short sampleBuffer[256];

// number of samples read
volatile int samplesRead;

#define lengthof(A) ((sizeof((A))/sizeof((A)[0])))

const PROGMEM uint8_t mouth_0[8][8] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {6,6,6,6,6,6,6,6},
    {6,6,6,6,6,6,6,6},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

const PROGMEM uint8_t mouth_4[8][8] = {
    {0,0,6,6,6,6,0,0},
    {0,6,0,0,0,0,6,0},
    {6,0,0,0,0,0,0,6},
    {6,0,0,0,0,0,0,6},
    {6,0,0,0,0,0,0,6},
    {6,0,0,0,0,0,0,6},
    {0,6,0,0,0,0,6,0},
    {0,0,6,6,6,6,0,0}
};

const PROGMEM uint8_t mouth_3[8][8] = {
    {0,0,0,0,0,0,0,0},
    {0,0,6,6,6,6,0,0},
    {0,6,0,0,0,0,6,0},
    {6,0,0,0,0,0,0,6},
    {6,0,0,0,0,0,0,6},
    {0,6,0,0,0,0,6,0},
    {0,0,6,6,6,6,0,0},
    {0,0,0,0,0,0,0,0}
};

const PROGMEM uint8_t mouth_2[8][8] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,6,6,6,6,6,6,0},
    {6,0,0,0,0,0,0,6},
    {6,0,0,0,0,0,0,6},
    {0,6,6,6,6,6,6,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

const PROGMEM uint8_t mouth_1[8][8] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,6,6,0,0,0},
    {6,6,6,0,0,6,6,6},
    {6,6,6,0,0,6,6,6},
    {0,0,0,6,6,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

const PROGMEM uint8_t mouth_smile[8][8] = {
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {6,0,0,0,0,0,0,6},
    {6,6,0,0,0,0,6,6},
    {0,6,6,6,6,6,6,0},
    {0,0,6,6,6,6,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

uint16_t palette[8] = {};
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 6,
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);


void drawImage(const uint8_t* image_addr){
    for(int x = 0; x<8; x++){
        for(int y = 0; y<8; y++){
            uint8_t index = pgm_read_byte(image_addr+x+y*8);
            matrix.drawPixel(x, y, palette[index]); 
        }
    }

    matrix.show();
}

void drawTest(void){
    for(int x = 0; x<8; x++){
      matrix.drawPixel(x, 0, palette[x]); 
    }
    matrix.show();
}

int pop_detection = 0;
bool smiling = false;
unsigned long smiletimer = 0;
unsigned long last_face = 0;

void setup() {
    Serial.begin(9600);
    
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
  
    matrix.begin();

    palette[0] = matrix.Color(0,0,0);
    palette[1] = matrix.Color(255,0,0);
    palette[2] = matrix.Color(0,255,0);
    palette[3] = matrix.Color(0,0,255);
    palette[4] = matrix.Color(0,255,255);
    palette[5] = matrix.Color(255,0,255);
    palette[6] = matrix.Color(255,255,0);
    palette[7] = matrix.Color(255,255,255);

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
        drawImage((const uint8_t*)mouth_smile);
    } else if(vol < 1){
        drawImage((const uint8_t*)mouth_0);
    } else if(vol < 3){
        drawImage((const uint8_t*)mouth_1);
    } else if(vol < 5){
        drawImage((const uint8_t*)mouth_2);
    } else if(vol < 10){
        drawImage((const uint8_t*)mouth_3);
    } else {
        drawImage((const uint8_t*)mouth_4);
    }
} 

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
