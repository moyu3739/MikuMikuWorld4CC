#pragma once

#include <windows.h>
#include <vector>

class Timer{
public:
    const std::vector<double> &record;

    // constructor
    Timer():record(_record){
        access=QueryPerformanceFrequency(&fre);
        active=false;
        _record.emplace_back(0);
    }

    // whether the timer is accessible
    bool Access(){
        return access;
    }

    // whether the timer is active
    bool Active(){
        return active;
    }

    // start the timer
    void StartTimer(){
        if(!access) return;
        active=true;
        ClearRecord();
        QueryPerformanceFrequency(&fre);
        QueryPerformanceCounter(&start);
    }

    // restart the timer
    void ReStartTimer(){
        StartTimer();
    }

    // end the timer
    // @return the time elapsed since the timer started
    double EndTimer(){
        if(!access) return -1;
        double end=RecordTimer();
        active=false;
        return end;
    }

    // read the timer
    // @return  the time elapsed since the timer started
    // @note  will not stop the timer
    double ReadTimer(){
        if(!access) return -1;
        if(active){
            LARGE_INTEGER curr;
            QueryPerformanceCounter(&curr);
            return double(curr.QuadPart-start.QuadPart)/fre.QuadPart;
        }
        else return _record.back();
    }

    // record the timer into `record`
    // @return  the time elapsed since the timer started
    double RecordTimer(){
        if(!access || !active) return -1;
        _record.emplace_back(ReadTimer());
        return _record.back();
    }

    // get the size of `record`
    int SizeOfRecord(){
        return _record.size();
    }

    // @param[in] i  the index of the element
    // @return  the i-th element of `record`
    // @note  if i < 0, return the i-th element from the end
    double CheckRecord(int i){
        if(i<0) i+=_record.size();
        return _record[i];
    }

    // clear `record`
    void ClearRecord(){
        _record.clear();
        _record.emplace_back(0);
    }

private:
    bool access; // if the timer is accessible
    bool active; // if the timer is active
    LARGE_INTEGER fre; // the frequency of the timer
    LARGE_INTEGER start; // the start time of the timer
    std::vector<double> _record; // the record of the timer
};
