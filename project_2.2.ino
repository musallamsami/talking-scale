#include "helper.h"
#include "scale.h"
#include "worker.h"
#include "audio.h"
#include "firestore.h"

#include <SoftwareSerial.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define API_KEY "AIzaSyAiE9hg0wrGLOPtctSv81nD_3mZnHOFWqI"
#define USER_EMAIL "admin@admin.com"
#define USER_PASSWORD "12345678"

SoftwareSerial RFID(21, 19);  // RX and TX

Audio audio;
FireStore firestore;
Worker worker("","","","");
Scale scale;

bool first_time_played;
String text;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;
static int8_t select_SD_card[] = { 0x7e, 0x03, 0X35, 0x01, (int8_t)0xef };  // 7E 03 35 01 EF

void LogOut();

void setup() {

    //PlayReadingTag(1)
    ///////////// RFID setup //////////////////////////
    Serial.begin(9600);
    RFID.begin(9600);
    pinMode(greenPin, OUTPUT);
    pinMode(redPin, OUTPUT);

    // Init MFRC522
    Serial.println("\nApproach your reader card...");
    Serial.println();
    start = false;

    ///////////////////// scale setup //////////////////////////
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    ///////////////////// sound setup //////////////////////////
    Serial1.begin(9600);
    audio.send_command_to_MP3_player(select_SD_card, 5);

    ///////////////////// wifi setup //////////////////////////
    ConnectToWifi();

    //PlayConnectingToWorkerData(2);
    /* Assign the api key (required) */
    firestore.config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    firestore.auth.user.email = USER_EMAIL;
    firestore.auth.user.password = USER_PASSWORD;

    /* Assign the callback function for the long running token generation task */
    firestore.config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

    Firebase.begin(&(firestore.config), &(firestore.auth));

    Firebase.reconnectWiFi(true);

    ///////////////////////Date & Time settup////////////////////////////
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    pinMode(23, INPUT_PULLUP);
    first_time_played = true;
    worker.language = "Arabic";
}
/******************************************************************/
void loop() 
{
    if (digitalRead(23) == LOW)
    {
        LogOut();
        while (RFID.available() > 0)
        {
            delay(5);
            c = RFID.read();
        }
    }

    ///////////////////// RFID read //////////////////////////
    while (!RFID.available() && !start){
        delay(5);
    }
    while (RFID.available() > 0 && !start){ //PlayConnectingToWorkerData(2.5);
        delay(5);
        c = RFID.read();
        text += c;
    }
    start = true;
    if (text.length() > 0){
        worker.id = "";
        for (int x = 1; (text[x] >= 'A' && text[x] <= 'Z') || (text[x] >= '0' && text[x] <= '9'); x++)
          worker.id += text[x];
        text = "";
    }
    if (first_time_played){
        String documentPath = "test/login";
        first_time_played = false;
        digitalWrite(redPin, HIGH);

        //PlayConnectingToWorkerData(3);
        ////////////////////////read from firestore/////////////////////////////////
        bool can_access_fire_store = firestore.CheckAccessFireStore(FIREBASE_PROJECT_ID,documentPath.c_str());
        if (!can_access_fire_store){
            audio.PlayEmailAndPasswordAreNotAuthenticated();
            return;
        }

        documentPath = StringSumHelper("Workers/") + worker.id;
        can_access_fire_store = firestore.CheckAccessFireStore(FIREBASE_PROJECT_ID,documentPath.c_str());
        if (can_access_fire_store){
            firestore.UpdateWorkerInfo(worker);
        }
        else{
            Serial.println(firestore.fbdo.errorReason());
        }

        if (unit < 0){
            audio.PlayCantCollectDataFromDB(first_time_played);
            return;
        }

        audio.sayIntro(worker.language);
        getTime(start_time);
    }

    ///////////////////// scale read //////////////////////////
    //PlayReadingScaleWeight(4)
    scale.ReadScaleWeight();
    scale.Run(audio,worker,firestore);

    delay(75);
}
/******************************************************************/

void LogOut(){
    unit = -1;
    start = false;
    first_time_played = true;

    digitalWrite(redPin, LOW);

    //int temp3 = play_indx_song[3];
    //int temp4 = play_indx_song[4];
    audio.PlayGoodBye(worker);
}

