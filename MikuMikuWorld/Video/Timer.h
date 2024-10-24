#pragma once

#include <windows.h>
#include <vector>
#include <exception>


class Timer{
public:
    // constructor
    Timer():records(_records){
        access = QueryPerformanceFrequency(&fre);
        active = false;
        _records.emplace_back(0);
    }

    // whether the timer is accessible
    bool Access() const{
        return access;
    }

    // whether the timer is active
    bool Active() const{
        return active;
    }

    void InaccessibleError() const{
        throw std::runtime_error("Timer is not accessible");
    }

    void InactiveError() const{
        throw std::runtime_error("Timer is not active");
    }

    // start the timer
    void StartTimer(){
        if(!access) InaccessibleError();
        active = true;
        ClearRecord();
        QueryPerformanceFrequency(&fre);
        QueryPerformanceCounter(&start);
    }

    // restart the timer
    void ReStartTimer(){
        StartTimer();
    }

    // end the timer
    // @param[in] final_record  whether to record the time finally
    // @return  if `final_record` true, return the time elapsed since the timer started;
    //          if `final_record` false, return 0
    double EndTimer(bool final_record = false){
        if(!access) InaccessibleError();
        active = false;
        if(final_record) return RecordTimer();
        else return 0;
    }

    // read the timer
    // @return  if timer active, return the time elapsed since the timer started;
    //          if timer inactive, return the last recorded time
    // @note  will not stop the timer
    double ReadTimer() const{
        if(!access) InaccessibleError();
        if(active){
            LARGE_INTEGER curr;
            QueryPerformanceCounter(&curr);
            return double(curr.QuadPart - start.QuadPart) / fre.QuadPart;
        }
        else return _records.back();
    }

    // record the timer into `record`
    // @return  the time elapsed since the timer started
    double RecordTimer(){
        if(!access) InaccessibleError();
        if(!active) InactiveError();
        _records.emplace_back(ReadTimer());
        return _records.back();
    }

    // get the number of `record`
    int RecordNumber(){
        return _records.size();
    }

    // @param[in] i  the index of the element
    // @return  the i-th element of `record`
    // @note  if i < 0, return the i-th element from the end
    double Record(int i) const{
        if(i<0) i+=_records.size();
        return _records[i];
    }

    // clear `record`
    void ClearRecord(){
        _records.clear();
        _records.emplace_back(0);
    }

public:
    const std::vector<double> &records;

private:
    bool access; // if the timer is accessible
    bool active; // if the timer is active
    LARGE_INTEGER fre; // the frequency of the timer
    LARGE_INTEGER start; // the start time of the timer
    std::vector<double> _records; // the record of the timer
};
