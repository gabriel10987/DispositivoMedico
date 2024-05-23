#include "Recordatorio.h"

Recordatorio::Recordatorio() : hora_(""), disponible_(0) {} // Constructor predeterminado vacío

Recordatorio::Recordatorio(const String& hora, int disponible) : hora_(hora), disponible_(disponible) {}

String Recordatorio::getHora() const {
    return hora_;
}

int Recordatorio::getDisponible() const {
    return disponible_;
}

void Recordatorio::setHora(const String& hora) {
    hora_ = hora;
}

void Recordatorio::setDisponible(int disponible) {
    disponible_ = disponible;
}
