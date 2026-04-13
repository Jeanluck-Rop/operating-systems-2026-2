#include <iostream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <cstring>

using namespace std;

#define RST  "\033[0m"
#define BOLD "\033[1m"
#define DIM  "\033[2m"
#define RED  "\033[91m"
#define GRN  "\033[92m"
#define YLW  "\033[93m"
#define BLU  "\033[94m"
#define CYN  "\033[96m"
#define WHT  "\033[97m"
#define MAG  "\033[95m"

void sep(const char* c = "-", const char* color = DIM) {
    cout << color;
    for (int i = 0; i < 62; i++) cout << c;
    cout << RST << "\n";
}

void cls() { cout << "\033[2J\033[H"; }

void pause() {
    cout << DIM << "\n  Presiona ENTER para continuar..." << RST;
    cin.ignore(10000, '\n');
}

/*  Base de datos simulada */
void showDatabase(const string& caller) {
    sep("=", RED);
    cout << RED << BOLD << "  BASE DE DATOS  |  banco_seguro.db\n" << RST;
    sep("=", RED);
    cout << RED << "  Accedido por: " << BOLD << caller << RST << "\n\n";
    cout << YLW
         << "  ID  Usuario       Nombre completo          Saldo        Direccion\n"
         << RST;
    sep("-", DIM);
    cout << "  01  " << WHT << "jperez" << RST
         << "        Juan Perez Lopez         " << GRN << "$2,450.00" << RST
         << "   Av. Reforma 142, CDMX\n";
    cout << "  02  " << WHT << "mgomez" << RST
         << "        Maria Gomez Ruiz          " << GRN << "$8,120.50" << RST
         << "   Calle 5 de Mayo 89, Puebla\n";
    cout << "  03  " << WHT << "cruiz" << RST
         << "         Carlos Ruiz Mendoza      " << GRN << "$45,000.00" << RST
         << "  Blvd. Kukulcan Km 12, Cancun\n";
    cout << "  04  " << WHT << "lhernandez" << RST
         << "    Laura Hernandez Vega      " << GRN << "$1,800.75" << RST
         << "   Calle Hidalgo 33, Monterrey\n";
    cout << "  05  " << WHT << "amorales" << RST
         << "      Alejandro Morales Gil    " << GRN << "$12,340.00" << RST
         << "  Av. Juarez 201, Guadalajara\n";
    sep("-", DIM);
    cout << RED << "  Tabla: credenciales\n\n" << RST;
    cout << YLW << "  Usuario       Hash contrasena                    Salt\n" << RST;
    sep("-", DIM);
    cout << "  jperez        " << DIM << "5f4dcc3b5aa765d61d8327de..." << RST << "  a3f9\n";
    cout << "  mgomez        " << DIM << "d8578edf8458ce06fbc5bb76..." << RST << "  b7c2\n";
    cout << "  cruiz         " << DIM << "96e79218965eb72c92a549dd..." << RST << "  e1a8\n";
    sep("-", DIM);
    cout << RED << "  Conexion : " << BOLD << "db-prod.bancoseguro.mx:5432\n" << RST;
    cout << RED << "  Clave AES: " << BOLD << "4f3a9c1b2e8d7f6a...\n" << RST;
    sep("=", RED);
}


/*  Clases de sesion con vtable */
class Session {
public:
    char username[64];
    double balance;

    Session(const string& u, double b) : balance(b) {
        strncpy(username, u.c_str(), 63);
        username[63] = '\0';
    }

    virtual string getType()  const { return "Session"; }

    virtual void viewDatabase() const {
        sep("=", RED);
        cout << RED << BOLD << "  ACCESO DENEGADO\n" << RST;
        cout << "  Se requiere nivel ADMIN para ver la base de datos.\n";
        sep("=", RED);
    }

    virtual void showMenu() const {
        sep();
        cout << BLU << BOLD << "  BANCO SEGURO S.A.  |  "
             << username << "  [CLIENT]\n" << RST;
        sep();
        cout << "  Saldo actual: " << GRN << "$" << fixed
             << setprecision(2) << balance << RST << "\n\n";
        cout << "  [1] Ver saldo\n"
             << "  [2] Transferir fondos\n"
             << "  [3] Ver base de datos del banco\n"
             << "  [4] Salir\n";
        sep();
    }

    virtual void transfer(const string& dest, double amount) {
        if (amount > 1000.0) {
            cout << RED << "  ERROR: limite de transferencia excedido ($1,000 max)\n" << RST;
            return;
        }
        if (amount > balance) {
            cout << RED << "  ERROR: saldo insuficiente\n" << RST;
            return;
        }
        balance -= amount;
        cout << GRN << "  Transferencia de $" << fixed << setprecision(2)
             << amount << " a [" << dest << "] completada.\n" << RST;
        cout << "  Saldo restante: $" << balance << "\n";
    }

    virtual ~Session() {}
};

// Jerarquia interna del admin: completamente separada de Session.
// Mismo layout de memoria (char[64] + double) para que el UAF
// funcione: el heap reutiliza bloques del mismo tamano.
class AdminBase {
public:
    virtual string getType()      const = 0;
    virtual void   viewDatabase() const = 0;
    virtual void   showMenu()     const = 0;
    virtual void   transfer(const string&, double) {}
    virtual ~AdminBase() {}
};


class AdminSession : public AdminBase {
public:
    char   username[64];
    double balance;

    AdminSession(const string& u) : balance(0) {
        strncpy(username, u.c_str(), 63);
        username[63] = '\0';
    }

    string getType() const override { return "AdminSession"; }

    void viewDatabase() const override {
        showDatabase(string(username));
    }

    void showMenu() const override {
        sep("=", MAG);
        cout << MAG << BOLD << "  BANCO SEGURO S.A.  |  "
             << username << "  [ADMIN]\n" << RST;
        sep("=", MAG);
        cout << "  [1] Ver base de datos completa\n"
             << "  [2] Ver logs del sistema\n"
             << "  [3] Cerrar sesion\n";
        sep("=", MAG);
    }

    ~AdminSession() {}
};

/*
 * Apuntado global al bloque liberado (simula la vulnerabilidad)
 * En un sistema real este puntero quedaria dentro de alguna
 * estructura interna del servidor que el atacante puede leer
 */
static void* g_freedBlock = nullptr; // direccion del bloque libre
static size_t g_blockSize = 0;       // tamano para verificar reutilizacion


/* Flujo admin */
void runAdminFlow() {
    cls();
    sep("=", MAG);
    cout << MAG << BOLD << "  BANCO SEGURO S.A.  |  Acceso Administrador\n" << RST;
    sep("=", MAG);

    cout << "\n  Usuario   : ";
    string u; getline(cin, u);
    cout << "  Contrasena: ";
    string p; getline(cin, p);

    if (u != "admin" || p != "admin") {
        cout << RED << "\n  Credenciales incorrectas.\n" << RST;
        pause();
        return;
    }

    cout << GRN << "\n  Acceso concedido. Bienvenido, " << u << ".\n" << RST;
    pause();

    AdminSession* adm = new AdminSession(u);
    g_blockSize = sizeof(AdminSession);

    cls();
    adm->showMenu();

    bool running = true;
    while (running) {
        cout << CYN << "  > " << RST;
        string opt; getline(cin, opt);

        if (opt == "1") {
            cls();
            adm->viewDatabase();
            pause();
            cls();
            adm->showMenu();
        } else if (opt == "2") {
            cout << DIM
                 << "\n  [LOG] 2025-04-12 09:14:32  login admin OK\n"
                 << "  [LOG] 2025-04-12 09:15:01  SELECT * FROM clientes\n"
                 << "  [LOG] 2025-04-12 09:16:44  UPDATE saldo WHERE id=03\n"
                 << RST;
            pause();
            cls();
            adm->showMenu();
        } else if (opt == "3") {
            running = false;
        }
    }

    cout << "\n" << YLW << "  [SISTEMA] Cerrando sesion admin...\n"          << RST;
    cout << DIM  << "  [MEMORIA] Objeto AdminSession en heap : "
         << (void*)adm << "\n"                                                << RST;

    g_freedBlock = (void*)adm;   // guardamos la direccion ANTES del delete
    delete adm;                  // BUG: bloque libre, puntero colgante

    cout << RED  << "  [MEMORIA] delete ejecutado. Bloque libre: "
         << g_freedBlock << "\n"                                              << RST;
    cout << DIM  << "  [MEMORIA] Ese bloque puede ser reasignado en el proximo new.\n" << RST;

    pause();
}

/* Flujo usuario normal */
void runUserFlow() {
    cls();
    sep("=", BLU);
    cout << BLU << BOLD << "  BANCO SEGURO S.A.  |  Acceso Cliente\n" << RST;
    sep("=", BLU);

    cout << "\n  Usuario   : ";
    string u; getline(cin, u);
    if (u.empty()) u = "usuario";
    cout << "  Contrasena: ";
    string p; getline(cin, p);

    cout << GRN << "\n  Acceso concedido. Bienvenido, " << u << ".\n" << RST;
    pause();

    Session* session = new Session(u, 2450.00);

    cls();
    session->showMenu();

    bool running = true;
    while (running) {
        cout << CYN << "  > " << RST;
        string opt; getline(cin, opt);

        if (opt == "1") {
            cout << "\n  Saldo actual: " << GRN << "$"
                 << fixed << setprecision(2) << session->balance << RST << "\n";
            pause();
            cls();
            session->showMenu();

        } else if (opt == "2") {
            cout << "\n  Destino (nombre o cuenta): ";
            string dest; getline(cin, dest);

            if (dest == "--admin-override") {
                cout << "\n" << YLW
                     << "  [SISTEMA] Procesando solicitud especial...\n"      << RST;
                cout << DIM
                     << "  [MEMORIA] Asignando buffer interno...\n"           << RST;

                // new AdminSession tiene el mismo tamano que el bloque
                // liberado del admin. El heap lo reutiliza.
                // Se construye sobre la memoria del bloque anterior.
                AdminSession* evil = new AdminSession(u);

                cout << RED << "  [EXPLOIT] Nuevo objeto en       : "
                     << (void*)evil       << "\n"                             << RST;
                cout << RED << "  [EXPLOIT] Bloque liberado estaba: "
                     << g_freedBlock      << "\n"                             << RST;

                if ((void*)evil == g_freedBlock) {
                    cout << RED << BOLD
                         << "  [UAF]     Mismo bloque reasignado.\n"          << RST;
                } else {
                    cout << YLW
                         << "  [UAF]     Bloque adyacente. vtable de AdminSession activa.\n"
                         << RST;
                }

                // UAF: reinterpretamos el puntero al objeto AdminSession
                // como si fuera un Session*. La vtable en esa posicion
                // apunta a AdminSession::viewDatabase.
                // Sin CFI: la llamada virtual despacha el metodo del admin.
                // Con CFI: la verificacion detecta que la vtable no es
                //          de tipo Session y aborta con SIGABRT.
                Session* dangling = reinterpret_cast<Session*>(evil);

                cout << RED
                     << "  [EXPLOIT] Puntero reinterpretado como Session*.\n"
                     << "  [EXPLOIT] vtable apunta a AdminSession::viewDatabase.\n"
                     << RST;

                pause();
                cls();

                cout << YLW << BOLD
                     << "  [SISTEMA] Verificando permisos via session->viewDatabase()...\n\n"
                     << RST;

                // Esta es la llamada que CFI intercepta:
                // dangling tiene tipo estatico Session* pero la vtable
                // real es de AdminSession (tipo incompatible segun CFI).
                dangling->viewDatabase();

                cout << "\n" << RED << BOLD
                     << "  Acceso a la base de datos obtenido sin credenciales admin.\n" << RST;
                cout << DIM
                     << "  El sistema creyo verificar permisos del cliente: "
                     << u << "\n"                                             << RST;

                pause();
                cls();
                session->showMenu();

                delete evil;

            } else {
                cout << "  Monto ($): ";
                string amtStr; getline(cin, amtStr);
                double amt = 0;
                try { amt = stod(amtStr); } catch (...) { amt = 0; }
                session->transfer(dest, amt);
                pause();
                cls();
                session->showMenu();
            }

        } else if (opt == "3") {
            cls();
            session->viewDatabase();
            pause();
            cls();
            session->showMenu();

        } else if (opt == "4") {
            running = false;
        }
    }

    delete session;
}

/* MAIN */
int main() {
    cls();
    sep("=", BLU);
    cout << BLU << BOLD << "  BANCO SEGURO S.A.\n" << RST;
    sep("=", BLU);
    cout << "\n"
         << "  [1] Acceso administrador\n"
         << "  [2] Acceso cliente\n"
         << "  [3] Salir\n\n";
    sep("=", BLU);
    cout << CYN << "  > " << RST;
    string opt; getline(cin, opt);

    if (opt == "1") {
        runAdminFlow();

        cls();
        sep("=", BLU);
        cout << BLU << BOLD << "  BANCO SEGURO S.A.\n" << RST;
        sep("=", BLU);
        cout << "\n  Sesion cerrada.\n\n"
             << "  [1] Acceso administrador\n"
             << "  [2] Acceso cliente\n"
             << "  [3] Salir\n\n";
        sep("=", BLU);
        cout << CYN << "  > " << RST;
        getline(cin, opt);
    }

    if (opt == "2") {
        runUserFlow();
    }

    cls();
    sep("=", GRN);
    cout << GRN << BOLD << "  Sesion terminada. Hasta luego.\n" << RST;
    sep("=", GRN);
    return 0;
}
