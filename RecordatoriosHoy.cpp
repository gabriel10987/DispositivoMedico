#include "RecordatoriosHoy.h"

RecordatoriosHoy::RecordatoriosHoy() : contadorKey_(0), contadorRespondido_(0) {}

void RecordatoriosHoy::agregarRecordatorio(const Recordatorio& recordatorio) {
    recordatoriosMap_[contadorKey_] = recordatorio;
    contadorKey_++;
}

void RecordatoriosHoy::responderRecordatorio() {
    if (contadorRespondido_ < contadorKey_) {
        // Buscar el recordatorio correspondiente al contadorRespondido_
        Recordatorio& recordatorio = recordatoriosMap_[contadorRespondido_];
        
        // Actualizar la disponibilidad
        recordatorio.setDisponible(0);
        
        // Incrementar contadorRespondido_
        contadorRespondido_++;
    }
}

Recordatorio RecordatoriosHoy::getRecordatorioPorKey(int key) {
    if (recordatoriosMap_.count(key) > 0) { // Verificar si la clave existe en el mapa
        return recordatoriosMap_[key]; // Devolver el recordatorio correspondiente a la clave
    } else {
        // Si la clave no existe, devolver un recordatorio vacío o manejar el error de otra manera
        return Recordatorio("", 0);
    }
}

int RecordatoriosHoy::contadorRespondido() {
    return contadorRespondido_;
}