#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>

// Representação do labirinto
using Maze = std::vector<std::vector<char>>;

// Estrutura para representar uma posição no labirinto
struct Position {
    int row;
    int col;
};

// Variáveis globais
Maze maze;
int num_rows;
int num_cols;
bool exit_found = false;

// Mutexes para sincronização
std::mutex maze_mutex;
std::mutex cout_mutex;
std::mutex exit_mutex;

// Função para carregar o labirinto de um arquivo
Position load_maze(const std::string& file_name) {
    std::ifstream infile(file_name);
    if (!infile) {
        std::cerr << "Erro ao abrir o arquivo " << file_name << std::endl;
        return {-1, -1};
    }

    infile >> num_rows >> num_cols;
    infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignora o resto da linha

    maze.resize(num_rows, std::vector<char>(num_cols));

    Position initial_pos = {-1, -1};
    for (int i = 0; i < num_rows; ++i) {
        std::string line;
        std::getline(infile, line);
        if (line.length() < num_cols) {
            std::cerr << "Linha " << i << " tem comprimento menor que o esperado." << std::endl;
            return {-1, -1};
        }
        for (int j = 0; j < num_cols; ++j) {
            maze[i][j] = line[j];
            if (maze[i][j] == 'e') {
                initial_pos.row = i;
                initial_pos.col = j;
            }
        }
    }

    if (initial_pos.row == -1 || initial_pos.col == -1) {
        std::cerr << "Entrada 'e' não encontrada no labirinto." << std::endl;
    }

    infile.close();
    return initial_pos;
}

// Função para imprimir o labirinto
void print_maze() {
    std::lock_guard<std::mutex> lock(cout_mutex);
    for (int i = 0; i < num_rows; ++i) {
        for (int j = 0; j < num_cols; ++j) {
            std::cout << maze[i][j];
        }
        std::cout << '\n';
    }
    std::cout << std::endl; // Adiciona uma linha em branco para melhor visualização
}

// Função para verificar se uma posição é válida
bool is_valid_position(int row, int col) {
    std::lock_guard<std::mutex> lock(maze_mutex);
    if (row >= 0 && row < num_rows && col >= 0 && col < num_cols) {
        char ch = maze[row][col];
        return (ch == 'x' || ch == 's');
    }
    return false;
}

// Função principal para navegar pelo labirinto
void walk(Position pos) {
    // Verifica se a saída já foi encontrada
    {
        std::lock_guard<std::mutex> lock(exit_mutex);
        if (exit_found) {
            return;
        }
    }

    // Bloqueia o labirinto para acessar e modificar
    {
        std::lock_guard<std::mutex> lock(maze_mutex);

        // Verifica se a posição atual é a saída
        if (maze[pos.row][pos.col] == 's') {
            {
                std::lock_guard<std::mutex> lock(exit_mutex);
                exit_found = true;
            }
            maze[pos.row][pos.col] = '.';

            print_maze();
            return;
        }

        // Marca a posição atual como visitada
        maze[pos.row][pos.col] = '.';
    }

    // Imprime o labirinto
    print_maze();

    // Pequeno atraso para visualização
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Direções: cima, baixo, esquerda, direita
    int row_moves[] = {-1, 1, 0, 0};
    int col_moves[] = {0, 0, -1, 1};

    std::vector<Position> next_positions;

    for (int i = 0; i < 4; ++i) {
        int new_row = pos.row + row_moves[i];
        int new_col = pos.col + col_moves[i];

        if (is_valid_position(new_row, new_col)) {
            // Bloqueia o labirinto para marcar posições
            {
                std::lock_guard<std::mutex> lock(maze_mutex);
                // Verifica novamente para garantir que a posição não foi visitada
                if (is_valid_position(new_row, new_col)) {
                    next_positions.push_back({new_row, new_col});
                    // Marca como 'o' temporariamente para evitar que outras threads a considerem
                    maze[new_row][new_col] = 'o';
                }
            }
        }
    }

    if (next_positions.empty()) {
        // Não há mais posições para explorar a partir daqui
        return;
    }

    // Se houver múltiplas posições, cria threads para posições adicionais
    std::vector<std::thread> threads;

    for (size_t i = 1; i < next_positions.size(); ++i) {
        // Cria uma nova thread para explorar next_positions[i]
        threads.emplace_back([pos = next_positions[i]]() {
            walk(pos);
        });
    }

    // Continua com a primeira posição nesta thread
    walk(next_positions[0]);

    // Aguarda as threads terminarem
    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_labirinto>" << std::endl;
        return 1;
    }

    Position initial_pos = load_maze(argv[1]);
    if (initial_pos.row == -1 || initial_pos.col == -1) {
        std::cerr << "Posição inicial não encontrada no labirinto." << std::endl;
        return 1;
    }

    // Inicia a exploração
    walk(initial_pos);

    if (exit_found) {
        std::cout << "Saída encontrada!" << std::endl;
    } else {
        std::cout << "Não foi possível encontrar a saída." << std::endl;
    }

    return 0;
}
