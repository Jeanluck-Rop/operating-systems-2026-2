# Bank UAF Demo

Simulacion de un sistema bancario con una vulnerabilidad
Use-After-Free real. El programa demuestra como un atacante
puede escalar privilegios de CLIENT a ADMIN sin conocer
credenciales, unicamente explotando un bug de manejo de memoria.

---

## Compilacion

Version vulnerable (el exploit funciona):

    clang++ -O0 -std=c++17 -o bank bank.cpp
    ./bank

Version protegida con CFI (el exploit aborta):

    clang++ -O1 -std=c++17 -fsanitize=cfi -flto -fvisibility=hidden -g -o bank_cfi bank.cpp
    ./bank_cfi

---
 
## Ejecucion: version vulnerable (./bank)
 
El siguiente flujo de inputs produce el ataque completo:
 
    > 1                       acceso administrador
    usuario:    admin
    contrasena: admin
    > 1                       ver base de datos     (datos visibles como admin)
    > 3                       cerrar sesion         (delete ocurre aqui)
 
    > 2                       acceso cliente
    usuario:    cualquiera    (ej: juan)
    contrasena: cualquiera    (ej: 1234)
    > 3                       ver base de datos     (ACCESO DENEGADO)
    > 2                       transferir fondos
    destino: --admin-override                       (la base de datos aparece completa)
 
---
 
## Ejecucion: version protegida (./bank_cfi)
 
El flujo de inputs es identico al anterior. La diferencia ocurre
al ingresar "--admin-override": CFI detecta que la vtable en el
slot de sesion no corresponde al tipo Session y el programa
termina antes de ejecutar ninguna instruccion del atacante.
 
    > 1
    usuario:    admin
    contrasena: admin
    > 1
    > 3
 
    > 2
    usuario:    cualquiera
    contrasena: cualquiera
    > 2
    destino: --admin-override
 
    Illegal instruction (core dumped)

---

## Flujo del programa

**PASO 1:** Menu principal
  El sistema presenta dos opciones de acceso: administrador o cliente.
  Para observar el ataque completo, primero se accede como admin.

**PASO 2:** Sesion administrador
  Credenciales validas: admin / admin
  El administrador puede consultar la base de datos completa,
  que contiene nombres, saldos, direcciones, hashes de contrasenas
  y credenciales de conexion a la base de datos de produccion.

**PASO 3:** Cierre de sesion admin
  Al cerrar sesion, el sistema ejecuta delete sobre el objeto
  AdminSession. El bloque de memoria queda libre pero la direccion
  permanece registrada internamente. Ahi reside la vulnerabilidad.

**PASO 4:** Sesion cliente
  Cualquier combinacion de usuario y contrasena es aceptada.
  El cliente tiene acceso limitado: puede ver su saldo, realizar
  transferencias hasta $1,000 y no puede consultar la base de datos.

**PASO 5:** Explotacion UAF
  Al realizar una transferencia, el campo destino acepta texto libre
  sin sanitizacion. Si se ingresa el valor "--admin-override", el
  sistema activa una ruta interna que reasigna el slot de sesion.
  Un nuevo objeto AdminSession es creado y el slot interno del
  sistema queda apuntando a el. La siguiente llamada a viewDatabase
  despacha AdminSession::viewDatabase en lugar de Session::viewDatabase.
  La base de datos completa es expuesta sin que el sistema detecte
  ninguna anomalia en el flujo de control.

---

## Por que funciona el ataque

El objeto AdminSession liberado en el paso 3 deja un slot interno
(g_sessionSlot) apuntando a memoria que ya no le pertenece al
programa. Cuando el exploit crea un nuevo AdminSession y redirige
ese slot, cualquier llamada virtual posterior ejecuta los metodos
del objeto malicioso. El sistema cree estar operando con la sesion
del cliente cuando en realidad despacha codigo de nivel administrador.

---

## Por que CFI lo detiene

Control-Flow Integrity instrumenta cada llamada virtual con una
verificacion de tipo en tiempo de ejecucion. Antes de despachar
viewDatabase, el compilador inserta una comprobacion que valida
que la vtable en esa direccion corresponde al tipo Session. Al
encontrar la vtable de AdminSession, la verificacion falla y el
programa termina con SIGABRT antes de ejecutar ninguna instruccion
del atacante.

Output al llegar al exploit con CFI activo:
    Illegal instruction (core dumped)
