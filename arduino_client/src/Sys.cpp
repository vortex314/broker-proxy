#include <Sys.h>
#include <Arduino.h>

uint64_t Sys::_upTime;
char Sys::_hostname[30] = "";
uint64_t Sys::_boot_time = 0;


uint64_t Sys::millis() // time in msec since boot, only increasing
{
    return ::millis();
}

uint32_t Sys::getFreeHeap() { return 0; }

void Sys::hostname(const char* hostname) {
    strncpy(_hostname, hostname, sizeof(_hostname));
}

const char* Sys::hostname() { return _hostname; }
void Sys::delay(uint32_t delta) {
    uint32_t end = Sys::millis() + delta;
    while (Sys::millis() < end)
        ;
}

uint64_t Sys::now() { return _boot_time + Sys::millis(); }

const char* Sys::board( ) {
    return S(BOARD);
}

const char* Sys::cpu(){
    return S(CPU);
}





