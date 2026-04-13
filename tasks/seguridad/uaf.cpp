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

//Clase maliciosa que simula lo que un atacante inyectaria en el heap
class Exploit {
public:
  virtual void speak() {
    cout << "[EXPLOIT]: *** CODIGO ATACANTE EN EJECUCION ***" << endl;
    cout << "[EXPLOIT]: Acceso autorizado. Ahora se puede escalar privilegios, abrir shell, exfiltrar datos..." << endl;
  }
  virtual ~Exploit() {}
};

int main() {
    cout << "\n=== ESTADO 1: SIN PROTECCIONES (UAF vulnerable) ===" << endl;

    //Paso 1: asignamos objeto legitimo en heap
    cout << "\n[PASO 1]: Creando objeto Chanchito en el heap..." << endl;
    Chanchito* chanchito = new Chanchito();
    chanchito->name = "Juantio y los Clonosaurios";
    cout << "         Chanchito creado en direccion: " << (void*)chanchito << endl;
    cout << "         vtable ptr apunta a: " << (void*)(*(uintptr_t*)chanchito) << endl;
    
    //Paso 2: llamada legitima
    cout << "\n[PASO 2]: Llamada legitima a speak():" << endl;
    chanchito->speak();
    
    //Paso 3: liberamos memoria (simula bug en el codigo)
    cout << "\n[PASO 3]: Liberando memoria con delete (bug en el programa)..." << endl;
    delete chanchito;
    cout << "         Memoria liberada. No obstante, el apuntador 'chanchito' sigue apuntando a: " << (void*)chanchito << endl;
    
    //Paso 4: el atacante rellena el hueco con objeto malicioso
    cout << "\n[PASO 4]: Atacante asigna objeto Exploit (mismo tamaño en heap)..." << endl;
    Exploit* exploit = new Exploit();
    cout << "         Exploit asignado en: " << (void*)exploit << endl;
    cout << "         nueva vtable ptr en esa direccion: " << (void*)(*(uintptr_t*)exploit) << endl;

    //Paso 5: el programa usa apuntero colgante -> ejecuta codigo del atacante
    cout << "\n[PASO 5] Programa usa el apuntador colgado chanchito->speak()..." << endl;
    cout << "         El apuntador colgado apunta a: " << (void*)chanchito << endl;
    cout << "         Que ahora contiene vtable de: Exploit" << endl;
    chanchito->speak(); // UAF: ejecuta Exploit::speak()

    delete exploit;
    return 0;
}
