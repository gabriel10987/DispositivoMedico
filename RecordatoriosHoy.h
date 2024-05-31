#ifndef RECORDATORIOS_HOY_H
#define RECORDATORIOS_HOY_H

#include <Arduino.h>
#include <map>
#include "Recordatorio.h"

class RecordatoriosHoy {
public:
    RecordatoriosHoy();

    int getContadorKey();
    int getContadorRespondido();
    std::map<int, Recordatorio> getRecordatoriosMap();

    void agregarRecordatorio(const Recordatorio& recordatorio);
    void responderRecordatorio();
    Recordatorio getRecordatorioPorKey(int key); 
        


private:
    std::map<int, Recordatorio> recordatoriosMap_;
    int contadorKey_;
    int contadorRespondido_;
};

#endif // RECORDATORIOS_HOY_H
