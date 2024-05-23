#ifndef RECORDATORIOS_HOY_H
#define RECORDATORIOS_HOY_H

#include <Arduino.h>
#include <map>
#include "Recordatorio.h"

class RecordatoriosHoy {
public:
    RecordatoriosHoy();

    void agregarRecordatorio(const Recordatorio& recordatorio);
    void responderRecordatorio();
    Recordatorio getRecordatorioPorKey(int key); 
        int contadorRespondido(); // Método para acceder a contadorRespondido_


private:
    std::map<int, Recordatorio> recordatoriosMap_;
    int contadorKey_;
    int contadorRespondido_;
};

#endif // RECORDATORIOS_HOY_H
