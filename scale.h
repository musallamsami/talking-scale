#ifndef SCALE_H
#define SCALE_H

#include "helper.h"
#include "audio.h"
#include "firestore.h"
#include <SoftwareSerial.h>

#define RXD2 16
#define TXD2 17

class Scale{
    public:
        String currWeightString;
        String oldWeightString;
        float oldVal;
        bool isNextJobReady;

    public:
        Scale() : currWeightString(""),oldWeightString(""), oldVal(0), isNextJobReady(true){}
        void ReadScaleWeight();
        void Run(Audio& audio, Worker& worker, FireStore& firestore);
        bool CheckIfTaskEnded(float& curr_val);
        bool CheckingIfShouldStartNewTask(float& curr_val);
        void PlayTaskEnded(Audio& audio, FireStore& firestore, Worker& worker,float& curr_val);
};

void Scale::ReadScaleWeight()
{
    oldWeightString = currWeightString;
    currWeightString = "";
    while (Serial2.available())
    {
        c = Serial2.read();
        currWeightString += c;
    }
}

void Scale::Run(Audio& audio, Worker& worker, FireStore& firestore)
{
    if (this->currWeightString == ""){
        return; //current val is 0
    }

    float curr_val = getNum(this->currWeightString);
    this->oldVal = this->oldVal * 0.50 + curr_val * 0.50;

    bool should_start_new_task = CheckingIfShouldStartNewTask(curr_val);
    if (should_start_new_task){
        audio.PlayShouldStartNewTask(curr_val,isNextJobReady,worker);
    }

    Serial.println("current weight: ");
    Serial.println(curr_val);

    bool is_task_ended = CheckIfTaskEnded(curr_val);
    if (is_task_ended){
        this->PlayTaskEnded(audio,firestore,worker,curr_val);
    }
    else{
        audio.sayNum((curr_val + unit / 2) / unit, worker.language, false);
        this->oldVal = -10;
        delay(1000);
    }
    currWeightString = "";
}

bool Scale::CheckIfTaskEnded(float& curr_val)
{
    return (curr_val > goal - unit / 2 && curr_val < goal + unit / 2);
}

bool Scale::CheckingIfShouldStartNewTask(float& curr_val){
    return (this->oldVal < curr_val + 0.01 && this->oldVal > curr_val - 0.01 && this->oldVal < goal * 0.7 && !(this->isNextJobReady));
}

void Scale::PlayTaskEnded(Audio& audio,FireStore& firestore, Worker& worker,float& curr_val){
    //task ended audio
    audio.play_indx_song[3] = 10;
    audio.play_indx_song[4] = 6;
    if (worker.language == "Hebrew") audio.play_indx_song[4] = 16;
    if (worker.language == "Russian") audio.play_indx_song[4] = 26;
    audio.send_command_to_MP3_player(audio.play_indx_song, 6);
    firestore.uploadData(curr_val,worker);
    this->oldVal = -10;
    this->isNextJobReady = false;
}
#endif //SCALE_H