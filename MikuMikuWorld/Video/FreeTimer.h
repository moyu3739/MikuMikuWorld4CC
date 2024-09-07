#pragma once

#include <iostream>
#include <exception>
#include "Timer.h"


class FreeTimer{
public:
    FreeTimer(double origin = 0.0){
        offset = origin;
        active = false;
        tm = new Timer();
    }

    ~FreeTimer(){
        delete tm;
    }

    // whether the timer is accessible
    bool Access(){
        return tm->Access();
    }

    // whether the timer is active
    bool Active(){
        return active;
    }

    // start the timer
    void Start(){
        active = true;
        tm->StartTimer();
    }

    // pause the timer
    void Pause(){
        if (active) {
            active = false;
            offset += tm->ReadTimer();
        }
    }

    // continue the timer
    void Continue(){
        if (!active){
            active = true;
            tm->StartTimer();
        }
    }

    // jump to a specific time
    void JumpTo(double seconds){
        offset = seconds;
        tm->StartTimer();
    }

    // read the timer
    double Read(){
        return active ? offset + tm->ReadTimer() : offset;
    }

private:
    Timer* tm;
    bool active;
    double offset;
};