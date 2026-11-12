#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <set>
#include <cctype>

using namespace std;

struct Jugador {
    int ranking_inicial;
    string nombres, apellidos, federacion;
    int elo;
    char sexo;
    int anio_nacimiento;
    double puntos = 0.0;
    vector<string> historial_enfrentamientos;
    double desempate1 = 0.0, desempate2 = 0.0;
    int desempate3 = 0;

    set<int> obtenerRivalesPrevios() const {
        set<int> rivales;
        for (const string& h : historial_enfrentamientos) {
            if (h == "BYE" || h.empty()) continue;
            size_t pos = h.find('w');
            if (pos == string::npos) pos = h.find('b');
            if (pos != string::npos && pos + 1 < h.length())
                rivales.insert(stoi(h.substr(pos + 1)));
        }
        return rivales;
    }

    char obtenerUltimoColor() const {
        if (historial_enfrentamientos.empty()) return ' ';
        for (auto it = historial_enfrentamientos.rbegin(); it != historial_enfrentamientos.rend(); ++it) {
            if (!it->empty() && *it != "BYE") {
                if (it->find('w') != string::npos) return 'w';
                if (it->find('b') != string::npos) return 'b';
            }
        }
        return ' ';
    }

    int contarTotalColor(char color) const {
        int count = 0;
        for (const string& h : historial_enfrentamientos) {
            if (!h.empty() && h != "BYE")
                if (h.find(color) != string::npos) count++;
        }
        return count;
    }
};

struct Partida {
    int mesa;
    Jugador *j1, *j2;
    double resultado_j1 = -1.0;
};

bool compararPorEloDesc(const Jugador& a, const Jugador& b) {
    return (a.elo != b.elo) ? a.elo > b.elo : a.apellidos < b.apellidos;
}

bool compararClasificacionOficial(const Jugador& a, const Jugador& b) {
    if (a.puntos != b.puntos) return a.puntos > b.puntos;
    if (a.desempate1 != b.desempate1) return a.desempate1 > b.desempate1;
    if (a.desempate2 != b.desempate2) return a.desempate2 > b.desempate2;
    if (a.desempate3 != b.desempate3) return a.desempate3 > b.desempate3;
    return a.elo > b.elo;
}

vector<Jugador> cargarJugadoresCSV() {
    vector<Jugador> lista;
    ifstream archivo("/workspaces/Chess-Swiss-Manager/Lista_de_jugadores.csv");

    if (!archivo.is_open()) {
        // Ruta alternativa local por si se ejecuta fuera de Colab
        archivo.open("Lista_de_jugadores.csv");
    }

    if (!archivo.is_open()) {
        cerr << "Error crítico: No se pudo abrir el archivo '/content/input/Lista_de_jugadores_256.csv'." << endl;
        return lista;
    }

    string linea;
    getline(archivo, linea); // Saltar cabecera
    while (getline(archivo, linea)) {
        if (linea.empty()) continue;
        stringstream ss(linea);
        string campo;
        Jugador j;
        getline(ss, j.nombres, ',');
        getline(ss, j.apellidos, ',');
        getline(ss, j.federacion, ',');
        getline(ss, campo, ','); j.elo = stoi(campo);
        getline(ss, campo, ','); j.sexo = campo.empty() ? ' ' : campo[0];
        getline(ss, campo, ','); j.anio_nacimiento = stoi(campo);
        lista.push_back(j);
    }
    archivo.close();
    return lista;
}

vector<Jugador> cargarJugadoresDesdeRonda(const string& nombreArchivo, int& rondaAnterior) {
    vector<Jugador> lista;
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) return lista;
    string linea;
    getline(archivo, linea);
    stringstream ssCabecera(linea);
    string col;
    int totalCols = 0;
    while (getline(ssCabecera, col, ',')) totalCols++;
    rondaAnterior = max(1, totalCols - 11);

    while (getline(archivo, linea)) {
        if (linea.empty()) continue;
        stringstream ss(linea);
        string campo;
        Jugador j;
        getline(ss, campo, ','); j.ranking_inicial = stoi(campo);
        getline(ss, j.nombres, ',');
        getline(ss, j.apellidos, ',');
        getline(ss, j.federacion, ',');
        getline(ss, campo, ','); j.elo = stoi(campo);
        getline(ss, campo, ','); j.sexo = campo.empty() ? ' ' : campo[0];
        getline(ss, campo, ','); j.anio_nacimiento = stoi(campo);
        getline(ss, campo, ','); j.puntos = stod(campo);
        for (int r = 0; r < rondaAnterior; ++r) {
            if (getline(ss, campo, ',') && !campo.empty())
                j.historial_enfrentamientos.push_back(campo);
        }
        lista.push_back(j);
    }
    return lista;
}

bool guardarJugadoresOrdenadosCSV(const string& nombreArchivo, const vector<Jugador>& lista) {
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) return false;
    archivo << "ranking_inicial,nombres,apellidos,federacion,elo,sexo,anio_nacimiento\n";
    for (const auto& j : lista) {
        archivo << j.ranking_inicial << "," << j.nombres << "," << j.apellidos << ","
                << j.federacion << "," << j.elo << "," << j.sexo << "," << j.anio_nacimiento << "\n";
    }
    return true;
}

void calcularDesempates(vector<Jugador>& jugadores) {
    vector<double> puntosPorRanking(jugadores.size() + 1, 0.0);
    for (const auto& j : jugadores) puntosPorRanking[j.ranking_inicial] = j.puntos;

    for (auto& j : jugadores) {
        vector<double> puntajesRivales;
        int victoriasNegras = 0;
        for (const string& h : j.historial_enfrentamientos) {
            if (h == "BYE" || h.empty()) continue;
            if (h.rfind("1b", 0) == 0) victoriasNegras++;
            size_t pos = h.find('w');
            if (pos == string::npos) pos = h.find('b');
            if (pos != string::npos && pos + 1 < h.length()) {
                int rivalId = stoi(h.substr(pos + 1));
                if (rivalId > 0 && (size_t)rivalId < puntosPorRanking.size())
                    puntajesRivales.push_back(puntosPorRanking[rivalId]);
            }
        }
        j.desempate3 = victoriasNegras;
        double sumaBuchholz = 0.0;
        for (double pRival : puntajesRivales) sumaBuchholz += pRival;
        j.desempate1 = sumaBuchholz;

        if (puntajesRivales.size() > 1) {
            auto minIt = min_element(puntajesRivales.begin(), puntajesRivales.end());
            double sumaCortado = 0.0;
            for (auto it = puntajesRivales.begin(); it != puntajesRivales.end(); ++it)
                if (it != minIt) sumaCortado += *it;
            j.desempate2 = sumaCortado;
        } else {
            j.desempate2 = sumaBuchholz;
        }
    }
}

void guardarResultadosRonda(int numRonda, const vector<Jugador>& jugadores) {
    string nombreArchivo = "ronda_" + to_string(numRonda) + "_resultados.csv";
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) return;

    archivo << "ranking_inicial,nombres,apellidos,federacion,elo,sexo,anio_nacimiento,puntos";
    size_t maxRondas = 0;
    for (const auto& j : jugadores)
        if (j.historial_enfrentamientos.size() > maxRondas) maxRondas = j.historial_enfrentamientos.size();
    for (size_t r = 1; r <= maxRondas; ++r) archivo << ",ronda_" << r;
    archivo << ",desempate_buchholz,desempate_buchholz_cortado,victorias_negras\n";

    for (const auto& j : jugadores) {
        archivo << j.ranking_inicial << "," << j.nombres << "," << j.apellidos << ","
                << j.federacion << "," << j.elo << "," << j.sexo << "," << j.anio_nacimiento << "," << j.puntos;
        for (const auto& hist : j.historial_enfrentamientos) archivo << "," << hist;
        for (size_t r = j.historial_enfrentamientos.size(); r < maxRondas; ++r) archivo << ",";
        archivo << "," << j.desempate1 << "," << j.desempate2 << "," << j.desempate3 << "\n";
    }
}

bool asignarBlancasA_J1(Jugador* j1, Jugador* j2) {
    char l1 = j1->obtenerUltimoColor(), l2 = j2->obtenerUltimoColor();
    int w1 = j1->contarTotalColor('w'), b1 = j1->contarTotalColor('b');
    int w2 = j2->contarTotalColor('w'), b2 = j2->contarTotalColor('b');

    int consW1 = 0, consB1 = 0;
    for (auto it = j1->historial_enfrentamientos.rbegin(); it != j1->historial_enfrentamientos.rend(); ++it) {
        if (*it == "BYE" || it->empty()) continue;
        if (it->find('w') != string::npos && consB1 == 0) consW1++;
        else if (it->find('b') != string::npos && consW1 == 0) consB1++;
        else break;
    }
    int consW2 = 0, consB2 = 0;
    for (auto it = j2->historial_enfrentamientos.rbegin(); it != j2->historial_enfrentamientos.rend(); ++it) {
        if (*it == "BYE" || it->empty()) continue;
        if (it->find('w') != string::npos && consB2 == 0) consW2++;
        else if (it->find('b') != string::npos && consW1 == 0) consB2++;
        else break;
    }

    if (consW1 >= 2) return false;
    if (consB1 >= 2) return true;
    if (consW2 >= 2) return true;
    if (consB2 >= 2) return false;

    int diff1 = w1 - b1, diff2 = w2 - b2;
    if (diff1 != diff2) return diff1 < diff2;
    if (l1 == 'b' && l2 != 'b') return true;
    if (l1 == 'w' && l2 != 'w') return false;
    return (j1->ranking_inicial % 2 != 0);
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    string archivoSalidaElo = "jugadores_ordenados_elo.csv";
    vector<Jugador> jugadores = cargarJugadoresCSV();
    if (jugadores.empty()) {
        cerr << "Error: No se pudo cargar el archivo de jugadores." << endl;
        return 1;
    }

    sort(jugadores.begin(), jugadores.end(), compararPorEloDesc);
    for (size_t i = 0; i < jugadores.size(); ++i) jugadores[i].ranking_inicial = i + 1;
    guardarJugadoresOrdenadosCSV(archivoSalidaElo, jugadores);

    int totalRondas = 0;
    cout << "Ingrese la cantidad total de rondas que se disputaran en el torneo: ";
    cin >> totalRondas;
    if (totalRondas < 1) totalRondas = 1;

    // Ronda 1
    int rondaActual = 1;
    cout << "\n====================================================================================\n";
    cout << "           GESTOR SUIZO - EMPAREJAMIENTOS RONDA 1 de " << totalRondas << "              \n";
    cout << "====================================================================================\n";

    size_t total = jugadores.size();
    bool tieneBye = (total % 2 != 0);
    Jugador* jugadorBye = nullptr;

    if (tieneBye) {
        jugadorBye = &jugadores.back();
        jugadorBye->puntos += 1.0;
        jugadorBye->historial_enfrentamientos.push_back("BYE");
        total--;
    }

    size_t mitad = total / 2;
    bool superiorJuegaConBlancas = (rand() % 2 == 0);
    vector<Partida> emparejamientosR1;

    for (size_t i = 0; i < mitad; ++i) {
        Partida p;
        p.mesa = i + 1;
        bool blancasArriba = (p.mesa % 2 != 0) ? superiorJuegaConBlancas : !superiorJuegaConBlancas;
        p.j1 = blancasArriba ? &jugadores[i] : &jugadores[i + mitad];
        p.j2 = blancasArriba ? &jugadores[i + mitad] : &jugadores[i];
        emparejamientosR1.push_back(p);
    }

    if (tieneBye && jugadorBye) {
        cout << ">>> Descanso (BYE) automático (+1 pt): " << jugadorBye->nombres << " " << jugadorBye->apellidos 
             << " (Rk: " << jugadorBye->ranking_inicial << ")\n\n";
    }

    cout << left << setw(6) << "Mesa" << setw(35) << "Blancas" << setw(35) << "Negras" << endl;
    cout << string(76, '-') << endl;
    for (const auto& p : emparejamientosR1) {
        cout << left << setw(6) << p.mesa
             << setw(35) << (p.j1->nombres + " " + p.j1->apellidos)
             << setw(35) << (p.j2->nombres + " " + p.j2->apellidos) << endl;
    }

    for (auto& p : emparejamientosR1) {
        int r = rand() % 3;
        p.resultado_j1 = (r == 0) ? 1.0 : (r == 1) ? 0.5 : 0.0;
        p.j1->puntos += p.resultado_j1;
        p.j2->puntos += (1.0 - p.resultado_j1);
        string r1 = (p.resultado_j1 == 1.0) ? "1" : (p.resultado_j1 == 0.5) ? "0.5" : "0";
        string r2 = (p.resultado_j1 == 0.0) ? "1" : (p.resultado_j1 == 0.5) ? "0.5" : "0";
        p.j1->historial_enfrentamientos.push_back(r1 + "w" + to_string(p.j2->ranking_inicial));
        p.j2->historial_enfrentamientos.push_back(r2 + "b" + to_string(p.j1->ranking_inicial));
    }

    calcularDesempates(jugadores);
    sort(jugadores.begin(), jugadores.end(), compararClasificacionOficial);
    guardarResultadosRonda(rondaActual, jugadores);

    string archivoUltimaRonda = "ronda_1_resultados.csv";

    // Bucle rondas subsiguientes
    while (rondaActual < totalRondas) {
        char continuar;
        cout << "\n¿Desea continuar a la siguiente ronda (Ronda " << (rondaActual + 1) << " de " << totalRondas << ")? (s/n): ";
        cin >> continuar;
        if (continuar != 's' && continuar != 'S') break;

        int rondaAnterior = 0;
        vector<Jugador> jugadoresRonda = cargarJugadoresDesdeRonda(archivoUltimaRonda, rondaAnterior);
        if (jugadoresRonda.empty()) break;

        calcularDesempates(jugadoresRonda);
        sort(jugadoresRonda.begin(), jugadoresRonda.end(), compararClasificacionOficial);

        rondaActual = rondaAnterior + 1;
        cout << "\n====================================================================================\n";
        cout << "             GESTOR SUIZO - RONDA " << rondaActual << " de " << totalRondas << "                        \n";
        cout << "====================================================================================\n";

        Jugador* jugadorByeRonda = nullptr;
        vector<Jugador*> activos;
        for (auto& j : jugadoresRonda) activos.push_back(&j);

        if (activos.size() % 2 != 0) {
            for (auto it = activos.rbegin(); it != activos.rend(); ++it) {
                bool yaTuvoBye = false;
                for (const string& h : (*it)->historial_enfrentamientos)
                    if (h == "BYE") { yaTuvoBye = true; break; }
                if (!yaTuvoBye) {
                    jugadorByeRonda = *it;
                    activos.erase(remove(activos.begin(), activos.end(), jugadorByeRonda), activos.end());
                    jugadorByeRonda->puntos += 1.0;
                    jugadorByeRonda->historial_enfrentamientos.push_back("BYE");
                    break;
                }
            }
        }

        vector<Partida> emparejamientos;
        vector<bool> emparejado(activos.size(), false);
        int numeroMesa = 1;

        for (size_t i = 0; i < activos.size(); ++i) {
            if (emparejado[i]) continue;
            Jugador* j1 = activos[i];
            set<int> rivales1 = j1->obtenerRivalesPrevios();
            int mejorOponenteIdx = -1;

            for (size_t j = i + 1; j < activos.size(); ++j) {
                if (!emparejado[j]) {
                    Jugador* j2 = activos[j];
                    if (rivales1.find(j2->ranking_inicial) == rivales1.end()) {
                        mejorOponenteIdx = j;
                        break;
                    }
                }
            }

            if (mejorOponenteIdx != -1) {
                Jugador* j2 = activos[mejorOponenteIdx];
                emparejado[i] = true;
                emparejado[mejorOponenteIdx] = true;
                Partida p;
                p.mesa = numeroMesa++;
                if (asignarBlancasA_J1(j1, j2)) { p.j1 = j1; p.j2 = j2; }
                else { p.j1 = j2; p.j2 = j1; }
                emparejamientos.push_back(p);
            }
        }

        for (size_t i = 0; i < activos.size(); ++i) {
            if (!emparejado[i]) {
                Jugador* j1 = activos[i];
                for (size_t j = i + 1; j < activos.size(); ++j) {
                    if (!emparejado[j]) {
                        Jugador* j2 = activos[j];
                        emparejado[i] = true;
                        emparejado[j] = true;
                        Partida p;
                        p.mesa = numeroMesa++;
                        if (asignarBlancasA_J1(j1, j2)) { p.j1 = j1; p.j2 = j2; }
                        else { p.j1 = j2; p.j2 = j1; }
                        emparejamientos.push_back(p);
                        break;
                    }
                }
            }
        }

        if (jugadorByeRonda) {
            cout << ">>> Descanso (BYE) automático (+1 pt): " << jugadorByeRonda->nombres << " " << jugadorByeRonda->apellidos 
                 << " (Rk: " << jugadorByeRonda->ranking_inicial << ")\n\n";
        }

        cout << left << setw(6) << "Mesa" << setw(35) << "Blancas (Ptos)" << setw(35) << "Negras (Ptos)" << endl;
        cout << string(76, '-') << endl;
        for (const auto& p : emparejamientos) {
            string bStr = p.j1->nombres + " " + p.j1->apellidos + " (" + to_string(p.j1->puntos) + ")";
            string nStr = p.j2->nombres + " " + p.j2->apellidos + " (" + to_string(p.j2->puntos) + ")";
            cout << left << setw(6) << p.mesa << setw(35) << bStr << setw(35) << nStr << endl;
        }

        for (auto& p : emparejamientos) {
            int r = rand() % 3;
            p.resultado_j1 = (r == 0) ? 1.0 : (r == 1) ? 0.5 : 0.0;
            p.j1->puntos += p.resultado_j1;
            p.j2->puntos += (1.0 - p.resultado_j1);
            string r1 = (p.resultado_j1 == 1.0) ? "1" : (p.resultado_j1 == 0.5) ? "0.5" : "0";
            string r2 = (p.resultado_j1 == 0.0) ? "1" : (p.resultado_j1 == 0.5) ? "0.5" : "0";
            p.j1->historial_enfrentamientos.push_back(r1 + "w" + to_string(p.j2->ranking_inicial));
            p.j2->historial_enfrentamientos.push_back(r2 + "b" + to_string(p.j1->ranking_inicial));
        }

        calcularDesempates(jugadoresRonda);
        sort(jugadoresRonda.begin(), jugadoresRonda.end(), compararClasificacionOficial);
        guardarResultadosRonda(rondaActual, jugadoresRonda);
        archivoUltimaRonda = "ronda_" + to_string(rondaActual) + "_resultados.csv";
    }

    // Tabla Final
    cout << "\n====================================================================================\n";
    cout << "                    TORNEO FINALIZADO - TABLA DE POSICIONES FINAL                   \n";
    cout << "====================================================================================\n";
    
    int ultimaRonda = 0;
    vector<Jugador> jugadoresFinales = cargarJugadoresDesdeRonda(archivoUltimaRonda, ultimaRonda);
    calcularDesempates(jugadoresFinales);
    sort(jugadoresFinales.begin(), jugadoresFinales.end(), compararClasificacionOficial);

    cout << left << setw(5)  << "Pos"
         << setw(22) << "Nombres"
         << setw(24) << "Apellidos"
         << setw(7)  << "Puntos"
         << setw(12) << "Buchholz"
         << setw(10) << "Cortado"
         << setw(8)  << "Vict.Neg" << endl;
    cout << string(88, '-') << endl;

    int puesto = 1;
    for (const auto& j : jugadoresFinales) {
        cout << left << setw(5)  << puesto++
             << setw(22) << j.nombres
             << setw(24) << j.apellidos
             << setw(7)  << j.puntos
             << setw(12) << j.desempate1
             << setw(10) << j.desempate2
             << setw(8)  << j.desempate3 << endl;
        if (puesto > 25) { 
            cout << "\n... [Mostrando los primeros 25 puestos de la clasificacion final en consola] ...\n";
            break;
        }
    }

    // Premiaciones y reconocimientos especiales
    cout << "\n====================================================================================\n";
    cout << "                         PREMIACIONES Y RECONOCIMIENTOS                             \n";
    cout << "====================================================================================\n";

    if (jugadoresFinales.size() >= 1) {
        cout << " 1er Puesto (Campeon): " << jugadoresFinales[0].nombres << " " << jugadoresFinales[0].apellidos 
             << " (" << jugadoresFinales[0].puntos << " pts)\n";
    }
    if (jugadoresFinales.size() >= 2) {
        cout << " 2do Puesto:          " << jugadoresFinales[1].nombres << " " << jugadoresFinales[1].apellidos 
             << " (" << jugadoresFinales[1].puntos << " pts)\n";
    }
    if (jugadoresFinales.size() >= 3) {
        cout << " 3er Puesto:          " << jugadoresFinales[2].nombres << " " << jugadoresFinales[2].apellidos 
             << " (" << jugadoresFinales[2].puntos << " pts)\n";
    }

    Jugador* mejorFemenino = nullptr;
    for (const auto& j : jugadoresFinales) {
        if (toupper(j.sexo) == 'F') {
            mejorFemenino = const_cast<Jugador*>(&j);
            break;
        }
    }
    if (mejorFemenino) {
        cout << " Mejor Jugadora:      " << mejorFemenino->nombres << " " << mejorFemenino->apellidos 
             << " (" << mejorFemenino->puntos << " pts)\n";
    } else {
        cout << " Mejor Jugadora:      No hubo competidoras femeninas registradas.\n";
    }

    Jugador* mejorSub20 = nullptr;
    for (const auto& j : jugadoresFinales) {
        int edad = 2026 - j.anio_nacimiento;
        if (edad < 20) {
            mejorSub20 = const_cast<Jugador*>(&j);
            break;
        }
    }
    if (mejorSub20) {
        cout << " Mejor Sub-20:        " << mejorSub20->nombres << " " << mejorSub20->apellidos 
             << " (Edad: " << (2026 - mejorSub20->anio_nacimiento) << ", " << mejorSub20->puntos << " pts)\n";
    } else {
        cout << " Mejor Sub-20:        No hubo jugadores menores de 20 años registrados.\n";
    }

    Jugador* mejorSub2600 = nullptr;
    for (const auto& j : jugadoresFinales) {
        if (j.elo < 2600) {
            mejorSub2600 = const_cast<Jugador*>(&j);
            break;
        }
    }
    if (mejorSub2600) {
        cout << " Mejor Sub-2600:      " << mejorSub2600->nombres << " " << mejorSub2600->apellidos 
             << " (Elo: " << mejorSub2600->elo << ", " << mejorSub2600->puntos << " pts)\n";
    } else {
        cout << " Mejor Sub-2600:      No hubo jugadores con Elo menor a 2600.\n";
    }

    cout << "\nTodos los registros y desempates finales estan guardados en: " << archivoUltimaRonda << "\n";
    return 0;
}
