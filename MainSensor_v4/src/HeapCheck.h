#include <Arduino.h>
#ifndef HeapCheckStart
#define HeapCheckStart millis()
#endif
#ifndef HeapLastCheck
#define HeapLastCheck 0
#endif

#ifndef heapcheck_H
#define heapcheck_H
void heapcheck(String location, int i){
    if(ESP.getFreeHeap()<10000){
        Serial.print("[W]Free heap (" + location + ") : ");
        Serial.printf("%d ( %d%% )\n", ESP.getFreeHeap(), ESP.getFreeHeap()/(double)ESP.getHeapSize()*100);
    }else if(millis() - HeapCheckStart >1000*60*10){
        Serial.print("[I]Free heap (" + location + ") : ");
        Serial.printf("%d ( %d%% )\n", ESP.getFreeHeap(), ESP.getFreeHeap()/(double)ESP.getHeapSize()*100);
        #undef HeapCheckStart
        #define HeapCheckStart millis()
    }else if(i == 1 || i == 2){
        Serial.print("[I]Free heap (" + location + ") : ");
        Serial.printf("%d ( %d%% )\n", ESP.getFreeHeap(), ESP.getFreeHeap()/(double)ESP.getHeapSize()*100);
    }else if(i == 2 && HeapLastCheck != 0){
        Serial.printf("[I]Heap different : %d\n", HeapLastCheck-ESP.getFreeHeap());
    }
    if(i != 0){
        #undef HeapLastCheck
        #define HeapLastCheck ESP.getFreeHeap()
    }
}

#endif