#ifndef RECORDATORIO_H
#define RECORDATORIO_H

#include <Arduino.h>

class Recordatorio {
public:
    Recordatorio(); // Constructor predeterminado
    Recordatorio(const String& hora, int disponible);

    // Métodos para acceder a los atributos
    String getHora() const;
    int getDisponible() const;

    // Métodos para actualizar los atributos
    void setHora(const String& hora);
    void setDisponible(int disponible);

private:
    String hora_;
    int disponible_;
};

#endif // RECORDATORIO_H
