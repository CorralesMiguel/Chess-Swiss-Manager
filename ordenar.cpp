#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

//Algoritmo para ordenar la lista inicial de jugadores

// Estructura para representar a cada participante del torneo
struct Jugador {
    int ranking_inicial;
    string nombres;
    string apellidos;
    string federacion;
    int elo;
    char sexo;
    int anio_nacimiento;
    double puntos = 0.0;
};

// Función comparadora: ordena de mayor a menor Elo.
// En caso de empate en Elo, desempata alfabéticamente por apellidos.
bool compararPorEloDesc(const Jugador& a, const Jugador& b) {
    if (a.elo != b.elo) {
        return a.elo > b.elo;
    }
    return a.apellidos < b.apellidos;
}

// 1. LECTURA: Lee el CSV desordenado y retorna un vector con los jugadores
vector<Jugador> cargarJugadoresCSV(const string& nombreArchivo) {
    vector<Jugador> lista;
    ifstream archivo("/workspaces/Chess-Swiss-Manager/Lista_de_jugadores.csv");

    if (!archivo.is_open()) {
        cerr << "Error: No se pudo abrir el archivo de entrada '" << nombreArchivo << "'." << endl;
        return lista;
    }

    string linea;
    // Leer y descartar la fila de encabezados
    getline(archivo, linea);

    while (getline(archivo, linea)) {
        if (linea.empty()) continue;

        stringstream ss(linea);
        string campo;
        Jugador j;

        getline(ss, j.nombres, ',');
        getline(ss, j.apellidos, ',');
        getline(ss, j.federacion, ',');

        getline(ss, campo, ',');
        j.elo = stoi(campo);

        getline(ss, campo, ',');
        j.sexo = campo.empty() ? ' ' : campo[0];

        getline(ss, campo, ',');
        j.anio_nacimiento = stoi(campo);

        lista.push_back(j);
    }

    archivo.close();
    return lista;
}

// 2. ESCRITURA: Guarda la lista ya ordenada en un nuevo archivo CSV
bool guardarJugadoresCSV(const string& nombreArchivo, const vector<Jugador>& lista) {
    ofstream archivo(nombreArchivo);

    if (!archivo.is_open()) {
        cerr << "Error: No se pudo crear el archivo de salida '" << nombreArchivo << "'." << endl;
        return false;
    }

    // Escribir la fila de encabezados
    archivo << "ranking_inicial,nombres,apellidos,federacion,elo,sexo,anio_nacimiento\n";

    // Escribir los datos de cada jugador ordenado
    for (const auto& j : lista) {
        archivo << j.ranking_inicial << ","
                << j.nombres << ","
                << j.apellidos << ","
                << j.federacion << ","
                << j.elo << ","
                << j.sexo << ","
                << j.anio_nacimiento << "\n";
    }

    archivo.close();
    return true;
}

int main() {
    string archivoEntrada = "jugadores_torneo_suizo_256-v2.csv";
    string archivoSalida  = "jugadores_ordenados_elo.csv";

    // Paso 1: Cargar datos desde el CSV
    vector<Jugador> jugadores = cargarJugadoresCSV(archivoEntrada);
    if (jugadores.empty()) {
        return 1;
    }

    // Paso 2: Ordenar de forma descendente por Elo
    sort(jugadores.begin(), jugadores.end(), compararPorEloDesc);

    // Paso 3: Asignar el número de ranking inicial (1 al 256)
    for (size_t i = 0; i < jugadores.size(); ++i) {
        jugadores[i].ranking_inicial = static_cast<int>(i + 1);
    }

    // Paso 4: Guardar la lista ordenada en un nuevo CSV
    if (guardarJugadoresCSV(archivoSalida, jugadores)) {
        cout << "Archivo '" << archivoSalida << "' generado correctamente con "
             << jugadores.size() << " registros.\n\n";
    }

    // Vista previa en consola de los primeros 15 jugadores
    cout << left << setw(5)  << "No."
         << setw(22) << "Nombres"
         << setw(24) << "Apellidos"
         << setw(6)  << "Fed"
         << setw(7)  << "Elo"
         << setw(6)  << "Sexo"
         << setw(6)  << "Nac." << endl;
    cout << string(76, '-') << endl;

    for (size_t i = 0; i < 15 && i < jugadores.size(); ++i) {
        const auto& j = jugadores[i];
        cout << left << setw(5)  << j.ranking_inicial
             << setw(22) << j.nombres
             << setw(24) << j.apellidos
             << setw(6)  << j.federacion
             << setw(7)  << j.elo
             << setw(6)  << j.sexo
             << setw(6)  << j.anio_nacimiento << endl;
    }

    return 0;
}
