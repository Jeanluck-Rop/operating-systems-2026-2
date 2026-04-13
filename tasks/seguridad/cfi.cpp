#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdint>

using namespace std;

//Clase Chanchito para representar el objetivo
class Chanchito {
public:
  const char* name;
  virtual void speak() {
    cout << "[Chanchito]: Holaa!! Soy un chanchito llamado " << name << " y estoy feliz." << endl;
  }
  virtual ~Chanchito() {}
};

// Clase maliciosa del atacante
class Exploit {
public:
  virtual void speak() {
    cout << "[EXPLOIT] *** ESTE MENSAJE NUNCA DEBERIA APARECER CON CFI ***" << endl;
  }
  virtual ~Exploit() {}
};

int main() {
  cout << "\n=== ESTADO 2: CON PROTECCIONES (AddressSanitizer + CFI) ===" << endl;
  
  cout << "\n[PASO 1]: Creando al Chanchito en el heap..." << endl;
  Chanchito* chanchito = new Chanchito();
  chanchito->name = "Juantio y los Clonosaurios";
  cout << "         Chanchito creado en direccion: " << (void*)chanchito << endl;
  cout << "         vtable ptr apunta a: " << (void*)(*(uintptr_t*)chanchito) << endl;

  cout << "\n[PASO 2]: Llamada legitima a speak():" << endl;
  chanchito->speak();
  
  cout << "\n[PASO 3]: Liberando memoria..." << endl;
  delete chanchito;
  cout << "         Memoria liberada." << endl;

  cout << "\n[PASO 4]: Atacante intenta inyectar Exploit en el heap..." << endl;
  Exploit* exploit = new Exploit();
  cout << "         Exploit en: " << (void*)exploit << endl;

  cout << "\n[PASO 5] Programa intenta usar puntero colgante animal->speak()..." << endl;
  cout << "         AddressSanitizer detectara el uso de memoria liberada." << endl;
  cout << "         CFI verificara que la vtable corresponde al tipo correcto." << endl;
  
  // Con -fsanitize=address esto aborta aqui con reporte detallado
  // Con -fsanitize=cfi esto aborta si la vtable no corresponde a Animal
  animal->speak();
  
  delete exploit;
  return 0;
}
